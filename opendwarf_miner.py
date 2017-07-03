#/usr/bin/env python
from opendwarf_miner_utils import *

#Benchmark parameters:
kmeans = {'name':'kmeans',
          'default':'-i ../test/dense-linear-algebra/kmeans/65536_34f.txt',#'204800.txt',
          'full name':'K-Means Clustering'}
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
gpu_parameters = GenerateDeviceParameters(2,0,1)

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
             {'name':'L1_data_cache_request_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L1_DCA')},
#L1 Data Cache Miss Rate
#PAPI_TOT_INS, PAPI_L1_DCM
             {'name':'L1_data_cache_miss_rate',
              'parameters':GeneratePAPIParameters('PAPI_TOT_INS', 'PAPI_L1_DCM')},
#L1 Data Cache Miss Ratio
#PAPI_L1_DCA PAPI_L1_DCM
             {'name':'L1_data_cache_miss_ratio',
              'parameters':GeneratePAPIParameters('PAPI_L1_DCA', 'PAPI_L1_DCM')},
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
              'parameters':GeneratePAPIParameters('PAPI_BR_INS', 'PAPI_BR_MSP')}
            ]

#so to find the cache performance steps of kmeans on i7 960@dynamic frequency,
#over the range of 1.60GHz-3.20GHz:
for papi_env in papi_envs:
    RunApplication(kmeans,cpu_parameters,10,papi_env['parameters'])
    StoreRun(kmeans,'results/kmeans_default_'+papi_env['name'])

#increasing the maximum number of clusters to find increases the amount of
#computation and thus the run time
#for max_clusters in range(1,12):
#    for papi_env in papi_envs:
#        RunApplicationWithArguments(kmeans,
#                                    kmeans['default']+" -n 1 -m "+str(max_clusters),
#                                    cpu_parameters,
#                                    30,#repeats
#                                    papi_env['parameters'])
#        StoreRun(kmeans,'results/kmeans_'+str(max_clusters)+"_max_clusters_"+papi_env['name'])

