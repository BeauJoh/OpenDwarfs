
import opentuner

from opentuner import ConfigurationManipulator
from opentuner import MeasurementInterface
from opentuner import IntegerParameter
from opentuner import Result
from opentuner.search.manipulator import PowerOfTwoParameter
from opentuner.search.objective import MinimizeTime
from pandas import read_csv
from os import remove

class WorkgroupSizeTuner(MeasurementInterface):
    def manipulator(self):
        manipulator = ConfigurationManipulator()
        manipulator.add_parameter(PowerOfTwoParameter('local1D',1,64))
        #manipulator.add_parameter(PowerOfTwoParameter('local2D_x',1,1024))
        #manipulator.add_parameter(PowerOfTwoParameter('local2D_y',1,1024))
        return manipulator

    def get_kernel_time(self,file_name,kernel_name):
        """
        measurement to collect a sum of all microseconds with the kernel name
        """
        df = read_csv(file_name,sep='\s+',comment='#')
        gross_time = int(sum(df[df['region']==kernel_name]['time']))
        return gross_time

    def get_kernel_cpu_energy(self,file_name,kernel_name):
        """
        measurement to collect a sum of all cpu nanojoules with the kernel name
        """
        df = read_csv(file_name,sep='\s+',comment='#')
        gross_time = int(sum(df[df['region']==kernel_name]['rapl:::PP0_ENERGY:PACKAGE0']))
        return gross_time

    def get_kernel_gpu_energy(self,file_name,kernel_name):
        """
        measurement to collect a sum of all gpu milliwatts with the kernel name
        """
        df = read_csv(file_name,sep='\s+',comment='#')
        gross_time = int(sum(df[df['region']==kernel_name]['nvml:::GeForce_GTX_1080:power']))
        return gross_time

    def lud_command(self, desired_result, cfg, input, limit):
        cpu_cmd = './lud -p 0 -d 0 --type 0 -x {0} -- -s 4096'.format(cfg['local1D'])
        gpu_cmd = './lud -p 1 -d 0 --type 1 -x {0} -- -s 4096'.format(cfg['local1D'])
        try:
            run_result = self.call_program(cpu_cmd)
        finally:
            assert run_result['returncode'] == 0
            time =  self.get_kernel_time('lsb.lud.r0','diagonal_kernel')
            time += self.get_kernel_time('lsb.lud.r0','perimeter_kernel')
            time += self.get_kernel_time('lsb.lud.r0','internal_kernel')
            remove('lsb.lud.r0')
            return Result(time=time)

    def run(self, desired_result, input, limit, trials=1):
        cfg = desired_result.configuration.data
        total = 0.0
        for _ in xrange(trials):
            total += self.lud_command(desired_result, cfg, input, limit).time
        return Result(time=total / trials)

    def save_final_config(self, configuration):
        self.manipulator().save_to_file(configuration.data,
                                        'lud_config.json')
    def objective(self):
        return MinimizeTime()

if __name__ == '__main__':
    argparser = opentuner.default_argparser()
    WorkgroupSizeTuner.main(argparser.parse_args())
