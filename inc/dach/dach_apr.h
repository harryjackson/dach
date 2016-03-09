#ifndef DACH_APR_H
#define DACH_APR_H
#pragma GCC system_header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#define DACH_APR_DEB 1
#ifdef  DACH_APR_DEB
#endif
/*
 * 1. No dach object will directly acesss apr.
 * 2. All APR access is done through one of 
 *    the interfaces here.
 *
 * This way I know what feature I need to replace if 
 * the we need to replace the APR or we can switch 
 * out the backend
 */


#pragma GCC diagnostic pop
#endif /*DACH_APR_H*/
