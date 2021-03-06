from __future__ import absolute_import, division, print_function
import six
import mmtbx.model
import iotbx.pdb
import boost_adaptbx.boost.python as bp
from libtbx.utils import null_out
from cctbx.array_family import flex
#
from cctbx.maptbx.box import shift_and_box_model

ext = bp.import_ext("cctbx_geometry_restraints_ext")

# ==============================================================================

def mon_lib_query(residue, mon_lib_srv):
    get_func = getattr(mon_lib_srv, "get_comp_comp_id", None)
    if (get_func is not None): return get_func(comp_id=residue)
    return mon_lib_srv.get_comp_comp_id_direct(comp_id=residue)

# ==============================================================================

def exclude_h_on_SS(model):
  rm = model.get_restraints_manager()
  bond_proxies_simple, asu = rm.geometry.get_all_bond_proxies(
    sites_cart = model.get_sites_cart())
  els = model.get_hierarchy().atoms().extract_element()
  ss_i_seqs = []
  all_proxies = [p for p in bond_proxies_simple]
  for proxy in asu:
    all_proxies.append(proxy)
  for proxy in all_proxies:
    if(  isinstance(proxy, ext.bond_simple_proxy)): i,j=proxy.i_seqs
    elif(isinstance(proxy, ext.bond_asu_proxy)):    i,j=proxy.i_seq,proxy.j_seq
    else: assert 0 # never goes here
    if([els[i],els[j]].count("S")==2): # XXX may be coordinated if metal edits used
      ss_i_seqs.extend([i,j])
  sel_remove = flex.size_t()
  for proxy in all_proxies:
    if(  isinstance(proxy, ext.bond_simple_proxy)): i,j=proxy.i_seqs
    elif(isinstance(proxy, ext.bond_asu_proxy)):    i,j=proxy.i_seq,proxy.j_seq
    else: assert 0 # never goes here
    if(els[i] in ["H","D"] and j in ss_i_seqs): sel_remove.append(i)
    if(els[j] in ["H","D"] and i in ss_i_seqs): sel_remove.append(j)
  return model.select(~flex.bool(model.size(), sel_remove))

# ==============================================================================

def exclude_h_on_coordinated_S(model): # XXX if edits used it should be like in exclude_h_on_SS
  rm = model.get_restraints_manager().geometry
  els = model.get_hierarchy().atoms().extract_element()
  # Find possibly coordinated S
  exclusion_list = ["H","D","T","S","O","P","N","C","SE"]
  sel_s = []
  for proxy in rm.pair_proxies().nonbonded_proxies.simple:
    i,j = proxy.i_seqs
    if(els[i] == "S" and not els[j] in exclusion_list): sel_s.append(i)
    if(els[j] == "S" and not els[i] in exclusion_list): sel_s.append(j)
  # Find H attached to possibly coordinated S
  bond_proxies_simple, asu = rm.get_all_bond_proxies(
    sites_cart = model.get_sites_cart())
  sel_remove = flex.size_t()
  for proxy in bond_proxies_simple:
    i,j = proxy.i_seqs
    if(els[i] in ["H","D"] and j in sel_s): sel_remove.append(i)
    if(els[j] in ["H","D"] and i in sel_s): sel_remove.append(j)
  return model.select(~flex.bool(model.size(), sel_remove))

# ==============================================================================

def add_missing_H_atoms_at_bogus_position(pdb_hierarchy, mon_lib_srv, protein_only):
  """
  Add missing H atoms to a pdb_hierarchy object
  all H atoms will be at center of coordinates (all of them superposed)
  ! this changes hierarchy in place !

  Parameters
  ----------
  pdb_hierarchy : cctbx hierarchy object
    pdb_hierarchy to which missing H atoms will be added
  """
  #XXX This breaks for 1jxt, residue 2, TYR

  get_class = iotbx.pdb.common_residue_names_get_class
  for m in pdb_hierarchy.models():
    for chain in m.chains():
      for rg in chain.residue_groups():
        for ag in rg.atom_groups():
          #print list(ag.atoms().extract_name())
          if(get_class(name=ag.resname) == "common_water"): continue
          #if(protein_only and
          #   not ag.resname.strip().upper() in aa_codes): continue
          if (protein_only and get_class(name=ag.resname) not in
            ["common_amino_acid", "modified_amino_acid"]): continue
          actual = [a.name.strip().upper() for a in ag.atoms()]
          mlq = mon_lib_query(residue=ag.resname, mon_lib_srv=mon_lib_srv)

          if (get_class(name=ag.resname) in ['modified_rna_dna', 'other']):
            #if mlq is None:
            continue

          expected_h = list()
          for k, v in six.iteritems(mlq.atom_dict()):
            if(v.type_symbol=="H"): expected_h.append(k)
          missing_h = list(set(expected_h).difference(set(actual)))
          if 0: print(ag.resname, missing_h)
          new_xyz = ag.atoms().extract_xyz().mean()
          hetero = ag.atoms()[0].hetero
          for mh in missing_h:
            # TODO: this should be probably in a central place
            if len(mh) < 4: mh = (' ' + mh).ljust(4)
            a = (iotbx.pdb.hierarchy.atom()
              .set_name(new_name=mh)
              .set_element(new_element="H")
              .set_xyz(new_xyz=new_xyz)
              .set_hetero(new_hetero=hetero))
            ag.append_atom(a)

# ==============================================================================

def add(model,
        use_neutron_distances = False,
        adp_scale             = 1,
        exclude_water         = True,
        protein_only          = False,
        stop_for_unknowns     = False,
        keep_existing_H       = False):
  """
  Add H atoms to a model

  Parameters
  ----------
  use_neutron_distances : bool
    use neutron distances instead of X-ray

  adp_scale : float
    scale factor for isotropic B of H atoms.
    B(H-atom) = adp_scale * B(parent non-H atom)

  keep_existing_H : bool
    keep existing H atoms in model, only place missing H

  Returns
  -------
  model
      mmtbx model object with H atoms
  """
  model_has_bogus_cs = False

  # TODO temporary fix until the code is moved to model class
  # check if box cussion of 5 A is enough to prevent symm contacts
  cs = model.crystal_symmetry()
  if cs is None:
    model = shift_and_box_model(model = model)
    model_has_bogus_cs = True

  # Remove existing H if requested
  if( not keep_existing_H):
    model = model.select(~model.get_hd_selection())

  pdb_hierarchy = model.get_hierarchy()
  mon_lib_srv = model.get_mon_lib_srv()
  """
  for pmodel in pdb_hierarchy.models():
    for chain in pmodel.chains():
      for residue_group in chain.residue_groups():
        for conformer in residue_group.conformers():
          for residue in conformer.residues():
            print list(residue.atoms().extract_name())
  """
  add_missing_H_atoms_at_bogus_position(pdb_hierarchy = pdb_hierarchy,
                                        mon_lib_srv   = mon_lib_srv,
                                        protein_only = protein_only)
  pdb_hierarchy.atoms().reset_serial()
  #pdb_hierarchy.sort_atoms_in_place()
  p = mmtbx.model.manager.get_default_pdb_interpretation_params()
  p.pdb_interpretation.clash_guard.nonbonded_distance_threshold=None
  p.pdb_interpretation.use_neutron_distances = use_neutron_distances
  p.pdb_interpretation.proceed_with_excessive_length_bonds=True
  #p.pdb_interpretation.restraints_library.cdl=False # XXX this triggers a bug !=360
  ro = model.get_restraint_objects()
  model = mmtbx.model.manager(
    model_input               = None,
    pdb_hierarchy             = pdb_hierarchy,
    build_grm                 = True,
    stop_for_unknowns         = stop_for_unknowns,
    crystal_symmetry          = model.crystal_symmetry(),
    restraint_objects         = ro,
    pdb_interpretation_params = p,
    log                       = null_out())

#  f = open("intermediate1.pdb","w")
#  f.write(model.model_as_pdb())

#  # Remove lone H
#  sel_h = model.get_hd_selection()
#  sel_isolated = model.isolated_atoms_selection()
#  sel_lone = sel_h & sel_isolated
#  model = model.select(~sel_lone)
#
  # Only keep H that have been parameterized in riding H procedure
  sel_h = model.get_hd_selection()
  model.setup_riding_h_manager(use_ideal_dihedral = True)
  sel_h_in_para = flex.bool(
    [bool(x) for x in model.riding_h_manager.h_parameterization])
  sel_h_not_in_para = sel_h_in_para.exclusive_or(sel_h)
  model = model.select(~sel_h_not_in_para)

  model = exclude_h_on_SS(model = model)
  model = exclude_h_on_coordinated_S(model = model)

#  f = open("intermediate2.pdb","w")
#  f.write(model.model_as_pdb())

  # Reset occupancies, ADPs and idealize
  model.reset_adp_for_hydrogens(scale = adp_scale)
  model.reset_occupancy_for_hydrogens_simple()
  model.idealize_h_riding()
  #
  return model

# ==============================================================================

# stub for reduce parameters
# TODO can be parameters or phil, depending on how many options are really needed
reduce_master_params_str = """
flip_NQH = True
  .type = bool
  .help = add H and rotate and flip NQH groups
search_time_limit = 600
  .type = int
  .help = max seconds to spend in exhaustive search (default=600)
"""

def optimize(model):
  """
  Carry out reduce optimization

  Parameters
  ----------
  model
      mmtbx model object that contains H atoms
      H atoms should be at approprite distances


  Returns
  -------
  model
      mmtbx model object with optimized H atoms
  """
  # hierarchy object --> has hierarchy of structure
  pdb_hierarchy = model.get_hierarchy()
  # geometry restraints manager --> info about ideal bonds, angles; what atoms are bonded, etc.
  grm = model.get_restraints_manager()

  print("Reduce optimization happens here")

  return model
