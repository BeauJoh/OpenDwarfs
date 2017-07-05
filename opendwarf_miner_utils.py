import os           #to get directory name of the calling script
import subprocess   #for exec
import glob         #for wildcards
import shutil       #to move files

def DMError(error_string):
    print("*"*80)
    print(" DwarfMiner Error: "+error_string)
    print("*"*80)

def GenerateDeviceParameters(platform_id,device_id,type_id):
    assert type(platform_id) is int, DMError("platform_id is not of type \'int\'")
    assert type(device_id) is int, DMError("device_id is not of type \'int\'")
    assert type(type_id) is int, DMError("type_id is not of type \'int\'")
    return(" -p {} -d {} -t {} -- ".format(platform_id,device_id,type_id))

def GeneratePAPIParameters(papi1,papi2):
    assert type(papi1) is str, DMError("papi1 is not of type \'str\'")
    assert type(papi2) is str, DMError("papi1 is not of type \'str\'")
    return("LSB_PAPI1="+papi1+' LSB_PAPI2='+papi2+" ")

def StoreRun(application,directory):
    assert type(directory) is str, DMError("directory is not of type \'str\'")
    i = 0
    while os.path.exists(directory+".{}".format(i)):
        i += 1
    numbered_directory = directory+".{}".format(i)

    os.makedirs(numbered_directory)
    run_name = application['name']
    for run in glob.glob('lsb.'+run_name+'.'+'*'):
        shutil.move(run,numbered_directory)

#def GenerateWorkgroupParameters(one_dimension,two_dimension_x=0,two_dimension_y=0):
#    assert type(one_dimension) is int, DMError("one_dimension is not of type \'int\'")
#    assert type(two_dimension_x) is int, DMError("two_dimension_x is not of type \'int\'")
#    assert type(two_dimension_y) is int, DMError("two_dimension_y is not of type \'int\'")
#    return("--workgroups \"-local1D {} -local2D {} {}\" ".format(one_dimension,two_dimension_x,two_dimension_y))

def RunApplicationWithArguments(application,arguments,parameters,repeats=1,papi_env=None):
    command = './'+application['name']+parameters+arguments
    if papi_env is not None:
        command = papi_env + command
    print('Running {}'.format(command))
    for i in range(0,repeats):
        if repeats != 1:
            print('iteration {}'.format(i))
        #process = subprocess.Popen(command,
        #                           shell=True,
        #                           stdout=subprocess.PIPE,
        #                           stderr=subprocess.PIPE)
        #
        #process.wait()
        #if process.returncode != 0:
        #    DMError('Application {} failed with {}'.format(application['name'],
        #        process.stderr.readlines()))
        try: 
            output = subprocess.check_output(command,shell=True)
        except subprocess.CalledProcessError as e:
            DMError('Application {} failed with {}'.format(application['name'], e.output))



def RunApplication(application,parameters,repeats=1,papi_env=None):
    RunApplicationWithArguments(application,
                                application['default'],
                                parameters,
                                repeats,
                                papi_env)

def RunDwarf(dwarf,parameters):
    assert type(dwarf) is list, DMError('dwarf is not of type \'list\'')
    for application in dwarf:
        RunApplication(application,parameters)

