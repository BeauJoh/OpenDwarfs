#if defined(TIME_WITH_LIBSCIBENCH)
#include <liblsb.h>
#else //define dummy functions that do nothing...
void LSB_Init(const char *projname, int autoprof_interval){}
void LSB_Finalize(){}
void LSB_Reg_param(const char *format, ...){}
void LSB_Set_Rparam_int(const char * id, int val){}
void LSB_Set_Rparam_long(const char * id, int64_t val){}
void LSB_Set_Rparam_string(const char * id, const char * val){}
void LSB_Set_Rparam_double(const char * id, double val){}
void LSB_Reg_id(const char *format, ...){}
double LSB_Rec(unsigned int id){}
double LSB_Check(unsigned int id){}
double LSB_Stop(unsigned int id, unsigned int reset){}
void LSB_Next(){}
void LSB_Flush(){}
void LSB_Res(){}
void LSB_Rec_enable(){}
void LSB_Rec_disable(){}
void LSB_Fold(unsigned int id, lsb_op_t op, double * result){}
double LSB_Wait(double microseconds){}
#endif
