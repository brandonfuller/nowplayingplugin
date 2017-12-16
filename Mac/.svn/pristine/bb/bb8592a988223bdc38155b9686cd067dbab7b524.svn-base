#import "Logger.h"

void MyNSLog1(const char * message)
{
    NSLog(@"%s", message);
}

void MyNSLog2(const char * message, CFStringRef string)
{
    NSLog(@"%s%@", message, string);
}

void MyNSLogv(const char * message, va_list args)
{
    NSLogv([NSString stringWithFormat:@"%s" , message], args);
}