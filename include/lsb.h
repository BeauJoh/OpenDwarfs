#if defined(TIME_WITH_LIBSCIBENCH)
#include <liblsb.h>
#else // declare dummy functions to avoid dependency on LibSciBench
#ifndef DUMMY_LIBLSB_H_
#define DUMMY_LIBLSB_H_
#if defined (__cplusplus)
extern "C" {
#endif
void LSB_Init(const char *projname, int autoprof_interval);
void LSB_Finalize();
void LSB_Reg_param(const char *format, ...);
void LSB_Set_Rparam_int(const char * id, int val);
void LSB_Set_Rparam_long(const char * id, int64_t val);
void LSB_Set_Rparam_string(const char * id, const char * val);
void LSB_Set_Rparam_double(const char * id, double val);
void LSB_Reg_id(const char *format, ...);
double LSB_Rec(unsigned int id);
double LSB_Check(unsigned int id);
double LSB_Stop(unsigned int id, unsigned int reset);
void LSB_Next();
void LSB_Flush();
void LSB_Res();
void LSB_Rec_enable();
void LSB_Rec_disable();
typedef enum { DUMMY } lsb_op_t;
void LSB_Fold(unsigned int id, lsb_op_t op, double * result);
double LSB_Wait(double microseconds);
#if defined (__cplusplus)
}
#endif
#endif /* DUMMY_LIBLSB_H_ */
#endif
