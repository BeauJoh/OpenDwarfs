#/usr/bin/env python
from opendwarf_miner_utils import *

#Benchmark parameters:
kmeans = {'name':'kmeans',
          'default':'-i ../test/dense-linear-algebra/kmeans/65536_34f.txt',
          'full name':'K-Means Clustering'}
kmeans_coarse_iteration_profile = {
        'name':'kmeans_profiling_outer_loop',
        'default':'-i ../test/dense-linear-algebra/kmeans/65536_34f.txt',
        'full name':'K-Means Clustering'}
kmeans_sparse = {'name':'kmeans',
                 'default':'-i ../test/dense-linear-algebra/kmeans/204800.txt',
                 'full name':'K-Means Clustering on Sparse Data'}
lud = {'name':'lud',
       'default':'-i ../test/dense-linear-algebra/lud/3072.dat',
       'full name':'Lower Upper Decomposition'}
csr = {'name':'csr',
       'default':'-i ../test/sparse-linear-algebra/SPMV/csr_65536.txt',
       'full name':'Compressed Sparse Row'}
fft = {'name':'clfft',
       'default':'--pts 1',
       'full name':'Fast Fourier Transform'}
gem = {'name':'gem',
       'default':"../test/n-body-methods/gem/nucleosome 80 1 0",
       'full name':'Gemnoui'}
srad = {'name':'srad',
        'default':'256 256 0 127 0 127 0.5 2',
        'full name':'Speckle Reducing Anisotropic Diffusion'}
cfd = {'name':'cfd',
       'default':'../test/unstructured-grids/cfd/fvcorr.domn.097K',
       'full name':'Computational Fluid Dynamics'}
crc = {'name':'crc',
       'default':'-i ../test/combinational-logic/crc/crc_1000x8192.txt',
       'full name':'Cyclic-Redundancy Check'}
bfs = {'name':'bfs',
       'default':'../test/graph-traversal/bfs/graph65536.txt',
       'full name':'Breadth-First Search'}
nw = {'name':'needle', 'default':"2048 10", 'full name':'Needleman-Wunsch'}
swat = {'name':'swat',
        'default':'../test/dynamic-programming/swat/query1K1 ../test/dynamic-programming/swat/sampledb1K1',
        'full name':'Smith-Waterman'}
nqueens = {'name':'nqueens',
           'default':'20',
           'full name':'N-Queens'}
hmm = {'name':'bwa_hmm',
       'default':'-n 30 -v n',
       'full name':'Baum-Welch Algorithm, Hidden Markov Model'}
tdm = {'name':'tdm',
       'default':'../test/finite-state-machine/tdm/sim-64-size200.csv ../test/finite-state-machine/tdm/ivl.txt ../test/finite-state-machine/tdm/30-episodes.txt 128',
       'full name':'Temporal Data Mining'}

#Dwarfs as clusters of Benchmarks:
dense_linear_algebra = [kmeans,lud]
sparse_linear_algebra = [csr]
spectral_methods = [fft]
n_body_methods = [gem]
structured_grid_methods = [srad]
unstructured_grid_methods = [cfd]
combinational_logic = [crc]
graph_traversal = [bfs]
dynamic_programming = [nw,swat]
backtrack_branch_and_bound = [nqueens]
graphical_models = [hmm]
finite_state_machines = [tdm]

#System specific device parameters
cpu_parameters = GenerateDeviceParameters(0,0,0)
gpu_parameters = GenerateDeviceParameters(1,0,1)

#Sample usage of utils:
#RunDwarf(dense_linear_algebra,cpu_parameters)

#Sample Usage with Papi to find L1 Cache performance:
#papi_env=GeneratePAPIParameters("PAPI_L1_TCA", #total L1 cache access
#                                "PAPI_L1_DCM") #L1 data cache misses
#RunApplication(kmeans,cpu_parameters,50,papi_env)
#StoreRun(kmeans,'kmeans_default_l1_cache_misses')

##Most common metrics that effect performance as PAPI events:
papi_envs = [
#Time (not a PAPI event, so just use whatever is the default)
             {'name':'time',
              'parameters':''},
#Instructions per cycle (IPC)
#PAPI_TOT_CYC, PAPI_TOT_INS
             {'name':'instructions_per_cycle',
              'parameters':GeneratePAPIParameters('PAPI_TOT_CYC', 'PAPI_TOT_INS')},
#L1 Data Cache Request Rate
#PAPI_TOT_INS, PAPI_L1_DCA
#             {'name':'L1_data_cache_request_rate',
#              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L1_DCA')},
#L1 Data Cache Miss Rate
#PAPI_TOT_INS, PAPI_L1_DCM
             {'name':'L1_data_cache_miss_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L1_DCM')},
#L1 Data Cache Miss Ratio
#PAPI_L1_DCA PAPI_L1_DCM
#             {'name':'L1_data_cache_miss_ratio',
#              'parameters':GeneratePAPIParameters('PAPI_L1_DCA', 'PAPI_L1_DCM')},
#L2 Data Cache Request Rate
#PAPI_TOT_INS PAPI_L2_DCA
             {'name':'L2_data_cache_request_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L2_DCA')},
#L2 Data Cache Miss Rate
#PAPI_TOT_INS PAPI_L2_DCM
             {'name':'L2_data_cache_miss_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L2_DCM')},
#L2 Data Cache Miss Ratio
#PAPI_L2_DCA PAPI_L2_DCM
             {'name':'L2_data_cache_miss_ratio',
              'parameters':GeneratePAPIParameters('PAPI_L2_DCA', 'PAPI_L2_DCM')},
#L3 Total Cache Request Rate
#PAPI_TOT_INS PAPI_L3_TCA
             {'name':'L3_total_cache_request_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L3_TCA')},
#L3 Total Cache Miss Rate
#PAPI_TOT_INS PAPI_L3_TCA
             {'name':'L3_total_cache_miss_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L3_TCM')},
#L3 Cache (can only use total cache instead of data cache events) Miss Ratio
#PAPI_L3_TCA PAPI_L3_TCM
             {'name':'L3_total_cache_miss_ratio',
              'parameters':GeneratePAPIParameters('PAPI_L3_TCA', 'PAPI_L3_TCM')},
#Translation Lookaside Buffer Misses:
#PAPI_TOT_INS PAPI_TLB_DM 
             {'name':'data_translation_lookaside_buffer_miss_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_TLB_DM')},
#Branch Rate:
#PAPI_TOT_INS PAPI_BR_INS
             {'name':'branch_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_BR_INS')},
#Branch Misprediction Rate:
#PAPI_TOT_INS PAPI_BR_INS
             {'name':'branch_misprediction_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_BR_MSP')},
#Branch Misprediction Ratio:
#PAPI_BR_INS PAPI_BR_MSP
             {'name':'branch_misprediction_ratio',
              'parameters':GeneratePAPIParameters('PAPI_BR_INS', 'PAPI_BR_MSP')},
 #RAPL Energy Measurements:
#rapl:::PP0_ENERGY:PACKAGE0, Energy used by all cores in package 0 (units nJ)
             {'name':'energy_nanojoules',
              'parameters':GeneratePAPIParameters('rapl:::PP0_ENERGY:PACKAGE0', 'rapl:::DRAM_ENERGY:PACKAGE0')},
            ]

##just get time (no papi events)
#papi_env = papi_envs[0]
##just get the energy
#papi_env = papi_envs[13]
#RunApplication(kmeans,cpu_parameters,5,papi_env['parameters'])
#StoreRun(kmeans,'results/kmeans_'+papi_env['name'])

#so to find the cache performance steps of kmeans on i7 960@dynamic frequency,
#over the range of 1.60GHz-3.20GHz:
#for papi_env in papi_envs:
#    RunApplication(kmeans,cpu_parameters,40,papi_env['parameters'])
#    StoreRun(kmeans,'results/kmeans_8_cores_'+papi_env['name'])

#increasing the maximum number of clusters to find increases the amount of
#computation and thus the run time
#selected_papi_envs = papi_envs
selected_papi_envs = []
selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'energy_nanojoules'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L1_data_cache_miss_rate'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L2_data_cache_miss_rate'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L3_total_cache_miss_rate'])

for max_clusters in range(1,15):
    for papi_env in selected_papi_envs:
        all_good = RunApplicationWithArguments(kmeans_coarse_iteration_profile,
                                               kmeans['default']+" -n 1 -m "+str(max_clusters),
                                               cpu_parameters,
                                               40,#repeats
                                               papi_env['parameters'])
        #all_good = RunApplicationWithArguments(kmeans,
        #                                       kmeans['default']+" -n 1 -m "+str(max_clusters),
        #                                       cpu_parameters,
        #                                       40,#repeats
        #                                       papi_env['parameters'])
        if all_good:
            StoreRun(kmeans,'results/kmeans_'+str(max_clusters)+"_max_clusters_"+papi_env['name'])
        else:
            import sys
            sys.exit()
#kmeans strider: increase the size of matrix to find to cache spillover sizes
#for n in range (0,25):
#    for papi_env in papi_envs:
#        all_good = RunApplicationWithArguments(kmeans,
#                                               "-i ../test/dense-linear-algebra/kmeans/{}_34f.txt".format(2**n),
#                                               cpu_parameters,
#                                               1,#repeats
#                                               papi_env['parameters'])
#        if all_good:
#            StoreRun(kmeans,'results/kmeans_'+str(2**n)+"_sized_matrix_"+papi_env['name'])
#        else:
#            import sys
#            sys.exit()

