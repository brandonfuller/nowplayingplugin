#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C"
{
#endif

void MyNSLog1(const char * message);
void MyNSLog2(const char * message, CFStringRef string);
void MyNSLogv(const char * message, va_list args);
    
#ifdef __cplusplus
}
#endif
