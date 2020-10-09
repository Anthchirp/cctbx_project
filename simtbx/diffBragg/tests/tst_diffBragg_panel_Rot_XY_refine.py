from argparse import ArgumentParser
parser = ArgumentParser()
parser.add_argument("--plot", action='store_true')
parser.add_argument("--residuals", action='store_true')
parser.add_argument("--oversample", type=int, default=0)
parser.add_argument("--curvatures", action="store_true")
parser.add_argument("--nopolar", action="store_true")
parser.add_argument("--setuponly", action="store_true")
parser.add_argument("--rotO", action="store_true")
parser.add_argument("--rotF", action="store_true")
parser.add_argument("--rotS", action="store_true")
parser.add_argument("--XY", action="store_true")
parser.add_argument("--toggle", action="store_true", help="if true, at the end of refinement, toggle before/after image")
args = parser.parse_args()

from dxtbx.model.crystal import Crystal
from IPython import embed
from cctbx import uctbx
from scitbx.matrix import sqr, rec, col
from dxtbx.model import Panel
from copy import deepcopy
import numpy as np
from scipy.spatial.transform import Rotation
from simtbx.diffBragg.refiners.local_refiner import LocalRefiner
from simtbx.diffBragg.refiners.crystal_systems import MonoclinicManager

from simtbx.diffBragg.nanoBragg_crystal import nanoBragg_crystal
from simtbx.diffBragg.sim_data import SimData
from simtbx.diffBragg import utils

ucell = (85.2, 96, 124, 90, 105, 90)
symbol = "P121"

# generate a random raotation
rotation = Rotation.random(num=1, random_state=1107)[0]
Q = rec(rotation.as_quat(), n=(4, 1))
rot_ang, rot_axis = Q.unit_quaternion_as_axis_and_angle()

# make the ground truth crystal:
a_real, b_real, c_real = sqr(uctbx.unit_cell(ucell).orthogonalization_matrix()).transpose().as_list_of_lists()
C = Crystal(a_real, b_real, c_real, symbol)
C.rotate_around_origin(rot_axis, rot_ang)

# Setup the simulation and create a realistic image
# with background and noise
# <><><><><><><><><><><><><><><><><><><><><><><><><>
nbcryst = nanoBragg_crystal()
nbcryst.dxtbx_crystal = C   # simulate ground truth
nbcryst.thick_mm = 0.1
nbcryst.Ncells_abc = 12, 12, 12

SIM = SimData()
SIM.detector = SimData.simple_detector(160, 0.1, (1024, 1024))

# grab the detector node (ground truth)
node = SIM.detector[0]
node_d = node.to_dict()
Origin = node_d["origin"][0], node_d["origin"][1], node_d["origin"][2]
distance = Origin[2]
gt_distance = distance
print("Ground truth originZ=%f" % (SIM.detector[0].get_origin()[2]))


# copy the detector and update the origin
#det2 = deepcopy(SIM.detector)
# alter the detector distance by 2 mm
#node_d["origin"] = Origin[0], Origin[1], Origin[2]+3
#embed()
#det2[0] = Panel.from_dict(node_d)
#print ("Modified originZ=%f" % (det2[0].get_origin()[2]))

SIM.crystal = nbcryst
SIM.instantiate_diffBragg(oversample=args.oversample)
SIM.D.progress_meter = False
SIM.D.verbose = 0 #1
SIM.D.nopolar = args.nopolar
SIM.water_path_mm = 0.005
SIM.air_path_mm = 0.1
SIM.add_air = True
SIM.add_Water = True
SIM.include_noise = True
#SIM.D.spot_scale = 1e8

SIM.D.add_diffBragg_spots()
spots = SIM.D.raw_pixels.as_numpy_array()
SIM._add_background()
SIM._add_noise()
# This is the ground truth image:
img = SIM.D.raw_pixels.as_numpy_array()
SIM.D.raw_pixels *= 0

# get the fast,slow, origin axis of the panel
from scitbx.matrix import col
panel = SIM.detector[0]
Ftru = col(panel.get_fast_axis())
Stru = col(panel.get_slow_axis())
Otru = col(panel.get_origin())

# Simulate the perturbed image for comparison
panel_rot_angO = 0.05
panel_rot_angF = 1
panel_rot_angS = 1
panel_rot_ang_radO = panel_rot_angO * np.pi / 180
panel_rot_ang_radF = panel_rot_angF * np.pi / 180
panel_rot_ang_radS = panel_rot_angS * np.pi / 180
Xshift_mm = 0.05
Yshift_mm = 0.05
#
## OFS rotations
#Frot = Ftru.rotate_around_origin(Stru, panel_rot_ang_radS).rotate_around_origin(Ftru, panel_rot_ang_radF).rotate_around_origin(Otru, panel_rot_ang_radO)
#Srot = Stru.rotate_around_origin(Stru, panel_rot_ang_radS).rotate_around_origin(Ftru, panel_rot_ang_radF).rotate_around_origin(Otru, panel_rot_ang_radO)
#
#
#from IPython import embed
#embed()
###################################################
###################################################
###################################################
###################################################
###################################################
#################################################
#################################################
#################################################
#################################################
#################################################
#
#from dxtbx.model import Experiment
#from simtbx.nanoBragg import make_imageset
#from cctbx_project.simtbx.diffBragg.phil import phil_scope
#from simtbx.diffBragg import refine_launcher
#E = Experiment()
#E.detector = SIM.detector
#E.beam = SIM.D.beam
#E.crystal = C
#E.imageset = make_imageset([img], E.beam, E.detector)
#
#refls = utils.refls_from_sims([spots], E.detector, E.beam, thresh=20)
#
#P = phil_scope.extract()
#P.roi.shoebox_size = 20
#P.roi.reject_edge_reflections = False
#P.refiner.refine_panelRotO = [1]
#P.refiner.refine_panelRotF = [1]
#P.refiner.refine_panelRotS = [1]
#P.refiner.max_calls = [1000]
#P.refiner.tradeps = 1e-10
## NOTE RUC.gtol = .9
## NOTE RUC.trad_conv = True  #False
## NOTE RUC.drop_conv_max_eps = 1e-9
#P.refiner.curvatures = False
#P.refiner.use_curvatures_threshold = 0
#P.refiner.poissononly = False
#P.refiner.verbose = True
#P.refiner.big_dump = False
#P.refiner.sigma_r = SIM.D.readout_noise_adu
#P.refiner.adu_per_photon = SIM.D.quantum_gain
#P.simulator.crystal.has_isotropic_ncells = True
#P.simulator.init_scale = SIM.D.spot_scale
#P.simulator.beam.size_mm = SIM.beam.size_mm
#
## assert RUC.all_ang_off[0] < 0.005
#RUC = refine_launcher.local_refiner_from_parameters(refls, E, P, miller_data=SIM.crystal.miller_array)
##assert round(RUC.D.get_value(9)) == 19
#print("OK")
#from IPython import embed
#embed()
#
###################################################
#################################################
#
#

#det2 = deepcopy(SIM.detector)
#panel = det2[0]
#orig = list(panel.get_origin())
#orig[0] += Xshift_mm
#orig[1] += Yshift_mm
#from dxtbx.model import Panel
#pan_dict = panel.to_dict()
#pan_dict["origin"] = orig
#pan = Panel.from_dict(pan_dict)
#det2[0] = pan

#SIM.D.update_dxtbx_geoms(det2, SIM.beam.nanoBragg_constructor_beam, 0,
#                         panel_rot_ang_radO, panel_rot_ang_radF, panel_rot_ang_radS)
#SIM.D.add_diffBragg_spots()
#SIM._add_background()
#SIM._add_noise()
# Perturbed image:
#img_pet = SIM.D.raw_pixels.as_numpy_array()
SIM.D.raw_pixels *= 0
full_roi = SIM.D.region_of_interest

# spot_rois, abc_init , these are inputs to the refiner
# <><><><><><><><><><><><><><><><><><><><><><><><><>
spot_roi, tilt_abc = utils.process_simdata(spots, img, thresh=20) #, plot=args.plot)
n_spots = len(spot_roi)
#n_kept = 30
np.random.seed(1)
idx = np.random.permutation(n_spots)#[:n_kept]
spot_roi = spot_roi[idx]
tilt_abc = tilt_abc[idx]
print ("I got %d spots!" % tilt_abc.shape[0])

#RUC = RefineDetdist(
#    spot_rois=spot_roi,
#    abc_init=tilt_abc,
#    img=img,
#    SimData_instance=SIM,
#    plot_images=args.plot,
#    plot_residuals=args.residuals)
#
#RUC.trad_conv = True
#RUC.refine_background_planes = False
#RUC.trad_conv_eps = 1e-5
#RUC.refine_detdist = True
#RUC.max_calls = 200
#RUC.run()
#
#
#
#print det2[0].get_origin()[2]
#print RUC.x[-3]
#
#assert abs(RUC.x[-3] - distance) < 1e-2
#
#print("OK!")
######3

nspot = len(spot_roi)

nanoBragg_rois = []  # special nanoBragg format
xrel, yrel, roi_imgs = [], [], []
xcom, ycom = [],[]
for i_roi, (x1, x2, y1, y2) in enumerate(spot_roi):
    nanoBragg_rois.append(((x1, x2), (y1, y2)))
    yr, xr = np.indices((y2 - y1 + 1, x2 - x1 + 1))
    xrel.append(xr)
    yrel.append(yr)
    roi_imgs.append(img[y1:y2 + 1, x1:x2 + 1])

    xcom.append(.5*(x1 + x2))
    ycom.append(.5*(x1 + x2))

q_spot = utils.x_y_to_q(xcom, ycom, SIM.detector, SIM.beam.nanoBragg_constructor_beam)
Ai = sqr(SIM.crystal.dxtbx_crystal.get_A()).inverse()
Ai = Ai.as_numpy_array()
HKL = np.dot(Ai, q_spot.T)
HKLi = [np.ceil(h - 0.5).astype(int) for h in HKL]
HKLi = [tuple(x) for x in np.vstack(HKLi).T]
Hi_asu = utils.map_hkl_list(HKLi, anomalous_flag=True, symbol=symbol)

nrotation_param = 3
nscale_param = 1
ntilt_param = 3*nspot
n_local_unknowns = nrotation_param + nscale_param + ntilt_param

UcellMan = MonoclinicManager(a=ucell[0], b=ucell[1], c=ucell[2], beta=ucell[4]*np.pi/180.)
nucell_param = len(UcellMan.variables)
n_ncell_param = 1
nfcell_param = len(Hi_asu)
ngain_param = 1
ndetz_param = 1
REFINE_XY = args.XY
REFINE_ROTO = args.rotO
REFINE_ROTF = args.rotF
REFINE_ROTS = args.rotS
n_panRot_param = 3
n_panXY_param = 2

n_global_unknowns = nucell_param + nfcell_param + ngain_param + ndetz_param + n_ncell_param + n_panRot_param + n_panXY_param
n_total_unknowns = n_local_unknowns + n_global_unknowns

SIM.D.oversample_omega = False
starting_originZ = SIM.detector[0].get_origin()[2]
RUC = LocalRefiner(
    n_total_params=n_total_unknowns,
    n_local_params=n_local_unknowns,
    n_global_params=n_global_unknowns,
    local_idx_start=0,
    shot_ucell_managers={0: UcellMan},
    shot_rois={0: spot_roi},
    shot_nanoBragg_rois={0: nanoBragg_rois},
    shot_roi_imgs={0: roi_imgs},
    shot_spectra={0: SIM.beam.spectrum},
    shot_crystal_GTs={0: C},
    shot_crystal_models={0: SIM.crystal.dxtbx_crystal},
    shot_xrel={0: xrel},
    shot_yrel={0: yrel},
    shot_abc_inits={0: tilt_abc},
    shot_asu={0: Hi_asu},
    global_param_idx_start=n_local_unknowns,
    shot_panel_ids={0: [0]*nspot},
    log_of_init_crystal_scales=None,
    all_crystal_scales=None,
    shot_originZ_init={0: starting_originZ},
    perturb_fcell=False,
    global_ncells=True,
    global_ucell=True,
    sgsymbol=symbol)

#TODO make this part of class init:
idx_from_asu = {h: i for i, h in enumerate(set(Hi_asu))}
asu_from_idx = {i: h for i, h in enumerate(set(Hi_asu))}

RUC.idx_from_asu = idx_from_asu
RUC.asu_from_idx = asu_from_idx

RUC.refine_background_planes =False
RUC.refine_Umatrix = False
RUC.refine_Bmatrix = False
RUC.refine_ncells =False
RUC.refine_crystal_scale = False
RUC.refine_Fcell = False
RUC.refine_detdist = False
RUC.refine_gain_fac = False
RUC.refine_panelRotO = REFINE_ROTO
RUC.refine_panelRotF = REFINE_ROTF
RUC.refine_panelRotS = REFINE_ROTS
RUC.refine_panelXY = REFINE_XY

RUC.ignore_line_search_failed_step_at_lower_bound = True

RUC.panel_group_from_id = {0: 0}  # panel group ID from panel id
dimX, dimY = SIM.detector[0].get_image_size()
ref = SIM.detector[0].get_pixel_lab_coord((dimX/2, dimY/2))
RUC.panel_reference_from_id = {0: SIM.detector[0].get_origin()}  # panel group ID from panel id

init_rotO = init_rotF = init_rotS = 0
if REFINE_ROTO:
    init_rotO = panel_rot_ang_radO
if REFINE_ROTF:
    init_rotF = panel_rot_ang_radF
if REFINE_ROTS:
    init_rotS = panel_rot_ang_radS
RUC.panelRot_init = {0: [init_rotO, init_rotF, init_rotS]}
#RUC.panelRot_init = {0: panel_rot_ang_rad}  # panel group ID versus starting value
RUC.panelRot_sigma = [1, 1, 1]

init_X_offset = init_Y_offset = 0
if REFINE_XY:
    init_X_offset = Xshift_mm
    init_Y_offset = Yshift_mm
RUC.panelX_init = {0: init_X_offset/1000.}
RUC.panelY_init = {0: init_Y_offset/1000.}
RUC.panelX_sigma = 1
RUC.panelY_sigma = 1
#RUC.n_panel_XY_param = 2 * int(REFINE_XY)

RUC.ucell_sigmas = [1]*len(UcellMan.variables)
RUC.ucell_inits = {0:  UcellMan.variables}

RUC.max_calls = 300
RUC.trad_conv_eps = 1e-5
RUC.trad_conv = True
RUC.trial_id = 0

RUC.plot_images = args.plot
RUC.plot_stride = 1
RUC.plot_spot_stride = 10
RUC.setup_plots()

RUC.rescale_params = True
RUC.refine_rotZ = False
RUC.request_diag_once = False
RUC.S = SIM
RUC.has_pre_cached_roi_data = True
RUC.S.D.update_oversample_during_refinement = False
RUC.S.D.nopolar = args.nopolar

RUC.use_curvatures = False
RUC.use_curvatures_threshold = 10
RUC.calc_curvatures = args.curvatures
RUC.poisson_only = True
RUC.verbose = True
RUC.big_dump = True

RUC.run(setup_only=args.setuponly)
#
#refined_distance = RUC._get_originZ_val(0)
#assert abs(refined_distance - distance) < 1e-2

#SIM.D.update_dxtbx_geoms(SIM.detector, SIM.beam.nanoBragg_constructor_beam, 0, *RUC._get_panelRot_val(0))
#RUC.update
#SIM.D.update_dxtbx_geoms(SIM.detector, SIM.beam.nanoBragg_constructor_beam, 0, RUC._get_panelRot_val(0))
rotO, rotF, rotS = map(lambda x: 180*x / np.pi, RUC._get_panelRot_val(0))
x, y = RUC._get_panelXY_val(0)
print("RotO=%.8f" % rotO)
print("RotF=%.8f" % rotF)
print("RotS=%.8f" % rotS)
print("panX=%.8f" % x)
print("panY=%.8f" % y)

if args.toggle:
    RUC._panel_id = 0
    RUC._i_shot = 0
    RUC._update_dxtbx_detector()
    RUC.S.D.raw_pixels *= 0
    RUC.S.D.region_of_interest = full_roi
    RUC.S.D.add_diffBragg_spots()
    RUC.S._add_background()
    RUC.S._add_noise()
    ## Perturbed image:
    img_opt = SIM.D.raw_pixels.as_numpy_array()
    from itertools import cycle
    imgs = cycle([img, img_opt])
    import pylab as plt
    plt.imshow(next(imgs), vmax=100)
    counter = 0
    while counter < 10:
        plt.draw()
        plt.pause(0.5)
        plt.cla()
        plt.imshow(next(imgs), vmax=100)
        plt.xlim(0,200)
        plt.ylim(200,0)
        counter += 1

assert rotO < 0.01
assert rotF < 0.01
assert rotS < 0.01
assert abs(x) < 0.0025/1000
assert abs(y) < 0.0025/1000

print("I AM ZIM")
print("OK!")
