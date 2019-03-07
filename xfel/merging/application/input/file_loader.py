from __future__ import division
import glob, os
from dxtbx.model.experiment_list import ExperimentListFactory
from libtbx import easy_pickle

try:
  import resource
  import platform
  def get_memory_usage():
    # getrusage returns kb on linux, bytes on mac
    units_per_mb = 1024
    if platform.system() == "Darwin":
      units_per_mb = 1024*1024
    return ('Memory usage: %.1f MB' % (int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss) / units_per_mb))
except ImportError:
  def debug_memory_usage():
    pass


"""
Utility functions used for reading input data
"""

class file_lister(object):
  """ Class for matching jsons to reflection table pickles """
  def __init__(self, params):
    self.params = params

  def filepair_generator(self):
    """ Used by rank 0, client/server to yield a list of json/reflection table pairs """
    for pathstring in self.params.input.path:
      for path in glob.glob(pathstring):
        if os.path.isdir(path):
          for filename in os.listdir(path):
            if filename.endswith(self.params.input.reflections_suffix):
              experiments_path = os.path.join(path, filename.split(self.params.input.reflections_suffix)[0] +
                 self.params.input.experiments_suffix)
              if not os.path.exists(experiments_path): continue
              yield experiments_path, os.path.join(path, filename)
        else:
          dirname = os.path.dirname(path)
          filename = os.path.basename(path)
          if filename.endswith(self.params.input.reflections_suffix):
            experiments_path = os.path.join(dirname, filename.split(self.params.input.reflections_suffix)[0] +
               self.params.input.experiments_suffix)
            if not os.path.exists(experiments_path): continue
            yield experiments_path, path

  def filename_lister(self):
    """ Used by rank 0, striping """
    return list(self.filepair_generator())

from xfel.merging.application.worker import worker
from xfel.merging.application.input.data_counter import data_counter

class simple_file_loader(worker):
  '''A class for running the script.'''

  def get_list(self):
    """ Read the list of json/reflection table pickle pairs """
    lister = file_lister(self.params)

    file_list = list(lister.filepair_generator())

    return file_list

  def run(self, all_experiments, all_reflections):
    """ Load all the data using MPI """
    from dxtbx.model.experiment_list import ExperimentList
    from dials.array_family import flex

    # Both must be none or not none
    test = [all_experiments is None, all_reflections is None].count(True)
    assert test in [0,2]
    if test == 2:
      all_experiments = ExperimentList()
      all_reflections = flex.reflection_table()
      starting_expts_count = starting_refls_count = 0
    else:
      starting_expts_count = len(all_experiments)
      starting_refls_count = len(all_reflections)

    # Generate and send a list of file paths to each worker
    if self.mpi_helper.rank == 0:
      file_list = self.get_list()
      self.params.input.path = None # the input is already parsed

      from xfel.merging.application.input.file_load_calculator import file_load_calculator
      load_calculator = file_load_calculator(self.params, file_list)
      calculated_file_list = load_calculator.calculate_file_load(self.mpi_helper.size)
      self.logger.log('Transmitting a list of %d lists of file pairs'%(len(calculated_file_list)))
      transmitted = calculated_file_list
    else:
      transmitted = None

    self.logger.log_step_time("BROADCAST_FILE_LIST")

    transmitted = self.mpi_helper.comm.bcast(transmitted, root = 0)

    new_file_list = transmitted[self.mpi_helper.rank]

    self.logger.log("Received a list of %d file pairs"%len(new_file_list))
    self.logger.log_step_time("BROADCAST_FILE_LIST", True)

    # Read the data

    # TODO: explore whether it makes sense to filter out experiments simultaneously with loading the data
    #from xfel.merging.application.filter.experiment_filter import experiment_filter
    #exp_filter = experiment_filter(self.params)

    self.logger.log_step_time("LOAD")
    for experiments_filename, reflections_filename in new_file_list:
      experiments = ExperimentListFactory.from_json_file(experiments_filename, check_format = False)
      reflections = easy_pickle.load(reflections_filename)

      #experiments, reflections = exp_filter.run(experiments, reflections)

      for experiment_id, experiment in enumerate(experiments):
        refls = reflections.select(reflections['id'] == experiment_id)
        all_experiments.append(experiment)
        refls['id'] = flex.int(len(refls), len(all_experiments)-1)
        all_reflections.extend(refls)
    self.logger.log_step_time("LOAD", True)

    self.logger.log('Read %d experiments consisting of %d reflections'%(len(all_experiments)-starting_expts_count, len(all_reflections)-starting_refls_count))
    self.logger.log(get_memory_usage())

    # Do statistics on the loaded data
    data_counter(self.params).count(all_experiments, all_reflections)

    return all_experiments, all_reflections

if __name__ == '__main__':
  from xfel.merging.application.worker import exercise_worker
  exercise_worker(simple_file_loader)
