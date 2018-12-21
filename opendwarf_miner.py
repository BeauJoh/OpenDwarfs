#/usr/bin/env python2
from opendwarf_miner_utils import *

exec(open('../opendwarf_application_parameters.py').read())
#execfile('../opendwarf_application_parameters.py')

from sys import argv,exit
selected_device = 0
selected_applications = None
selected_problem_sizes = ['tiny',
                          'small',
                          #'medium',
                          #'large',
                          ]
selected_iterations = 50
if len(argv) == 5:
    selected_device = int(argv[1])
    selected_applications = str(argv[2])
    selected_problem_sizes = [str(argv[3])]
    selected_iterations = int(argv[4])
    print("Using device:"+str(selected_device)+"on application:"+selected_applications+"on size"+str(selected_problem_sizes[0]))

#System specific device parameters
device_parameters = None
device_name = None
import socket
if socket.gethostname() == "Beaus-MacBook-Air.local":
    device_name = "intel_hd_graphics_5000"
    device_parameters = GenerateDeviceParameters(0,1,1)#Intel HD Graphics 5000

if socket.gethostname() == "gpgpu":
    if selected_device == 0:
        device_name = "i7-6700k"
        device_parameters = GenerateDeviceParameters(0,0,0)#i7-6700K
    else:
        device_name = "gtx1080"
        device_parameters = GenerateDeviceParameters(1,0,1)#GTX 1080

if socket.gethostname() == "node03":
    device_name = "knl"
    device_parameters = GenerateDeviceParameters(0,0,2)#knights landing

if socket.gethostname() == "node33":
    device_name = "firepro-s9150"
    device_parameters = GenerateDeviceParameters(0,0,0)#firepro s9150

if socket.gethostname() == "node01":
    device_name = "xeon_es-2697v2"
    device_parameters = GenerateDeviceParameters(0,0,0)#ivybridge Xeon E5-2697v2

if socket.gethostname() == "node23":
    if selected_device == 0:
        device_name = "gtx1080ti"
        device_parameters = GenerateDeviceParameters(0,0,1)#gtx1080ti
    else:
        device_name = "titanx"
        device_parameters = GenerateDeviceParameters(0,0,1)#titanx

if socket.gethostname() == "node20":
    if selected_device == 0:
        device_name = "k20c"
        device_parameters = GenerateDeviceParameters(0,0,1)#k20c
    else:
        device_name = "k40c"
        device_parameters = GenerateDeviceParameters(0,0,1)#k40c

if socket.gethostname() == "node02":
    if selected_device == 0:
        device_name = "spectre"
        device_parameters = GenerateDeviceParameters(0,0,1)#spectre gpu
    else:
        device_name = "a10-780k"
        device_parameters = GenerateDeviceParameters(0,1,1)#AMD A10-7850K Radeon R7
if socket.gethostname() == "whale":
    if selected_device == 0:
        device_name = "p100"
        device_parameters = GenerateDeviceParameters(0,0,1)#P100
    else:
        device_name = "gold-6134"
        device_parameters = GenerateDeviceParameters(1,0,0)#xeon gold on whale

if socket.gethostname() == "node30":
    if selected_device == 0:
        device_name = "tahiti-hd7970"
        device_parameters = GenerateDeviceParameters(0,0,0)#hd7970
    elif selected_device == 1:
        device_name = "hawaii-r9-295x2"
        device_parameters = GenerateDeviceParameters(0,0,0)#r9-295x2
    else:
        device_name = "i5-3350"
        device_parameters = GenerateDeviceParameters(0,1,0)#Intel(R) Core(TM) i5-3550 CPU @ 3.30GHz

if socket.gethostname() == "node31":
    if selected_device == 0:
        device_name = "fiji-furyx"
        device_parameters = GenerateDeviceParameters(0,0,0)#hd7970
    elif selected_device == 1:
        device_name = "hawaii-r9-290x"
        device_parameters = GenerateDeviceParameters(0,0,0)#r9-295x2
    else:
        device_name = "i5-3350"
        device_parameters = GenerateDeviceParameters(0,1,0)#Intel(R) Core(TM) i5-3550 CPU @ 3.30GHz



if socket.gethostname() == "node32":
    if selected_device == 0:
        device_name = "polaris-rx480"
        device_parameters = GenerateDeviceParameters(0,0,0)#hd7970
    else:
        device_name = "i5-3350"
        device_parameters = GenerateDeviceParameters(0,1,1)#Intel(R) Core(TM) i5-3550 CPU @ 3.30GHz



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
             {'name':'cpu_energy_nanojoules',
              'parameters':GeneratePAPIParameters('rapl:::PP0_ENERGY:PACKAGE0', 'rapl:::DRAM_ENERGY:PACKAGE0')},
#nvml Energy Measurements:
#nvml:::GeForce_GTX_1080:power, Power usage readings for the device (units mW) in miliwatts. This is the power draw (+/-5 watts) for the entire board: GPU, memory etc.
#nvml:::GeForce_GTX_1080:temperature, current temperature readings for the device, in degrees C.
             {'name':'gpu_energy_milliwatts',
                     'parameters':GeneratePAPIParameters('nvml:::GeForce_GTX_1080:power', 'nvml:::GeForce_GTX_1080:temperature')},
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
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'cpu_energy_nanojoules'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'gpu_energy_milliwatts'])
selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'time'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L1_data_cache_miss_rate'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L2_data_cache_miss_rate'])
#selected_papi_envs.extend([x for x in papi_envs if x['name'] == 'L3_total_cache_miss_rate'])

if selected_applications == None:
    selected_applications = [
                             kmeans,
                             lud,
                             csr,
                             fft,
                             dwt,
                             gem,
                             srad,
                             crc,
                             bfs,
                             nw,
                             hmm,
                             nqueens,
                            ]
else:
    exec("%s = [%s]"%("selected_applications",selected_applications))

#selected_applications = [kmeans_coarse_iteration_profile]#kmeans]
#selected_applications = [fft]#csr,kmeans]
#selected_applications.extend(dense_linear_algebra)

#if running the whole list of dwarfs, we need to flatten the list first
#selected_applications = reduce(lambda x,y :x+y ,dwarfs)

selected_device = device_parameters

#instrument all applications
for application in selected_applications:
    for papi_env in selected_papi_envs:
        for problem_size in selected_problem_sizes:
            all_good = RunApplicationWithArguments(application,
                                                   application[str(problem_size)],
                                                   selected_device,
                                                   selected_iterations,
                                                   papi_env['parameters'])
            if all_good:
                StoreRun(application,
                        'results/'+device_name+'_'+application['alias']+'_'+problem_size+'_'+papi_env['name'])
            else:
                import ipdb
                ipdb.set_trace()
                #import sys
                #sys.exit()

##kmeans strider: increase the size of feature space to find to cache spillover sizes
#feature_sizes = [256,2048,65536,524288]
#repeats = 300
#for f in feature_sizes:
#    for o in range (5,48):
#        for papi_env in selected_papi_envs:
#             #'tiny':'-g -p 26 -f 256',     #< 32K, object increments of 1KiB
#             #'small':'-g -p 26 -f 2048',   #< 256K, object increments of 8KiB
#             #'medium':'-g -p 26 -f 65536', #< 8196K, object increments of 256KiB
#             #'large':'-g -p 26 -f 524288', #> 8196K, object increments of 2048KiB
#
#            all_good = RunApplicationWithArguments(kmeans,
#                                                   "-g -p {} -f {} -m 5 -n 5".format(o,f),
#                                                   cpu_parameters,
#                                                   repeats,
#                                                   papi_env['parameters'])
#            if all_good:
#                StoreRun(kmeans,"results/kmeans_{}_objects_{}_features_{}".format(o,f,papi_env['name']))
#            else:
#                import sys
#                sys.exit()

