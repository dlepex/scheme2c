/* __libr.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef __LIBR_H_
#define __LIBR_H_

/* BIN initialization*/
void bin_init(svm_t vm)
{
	extern void libr_init(svm_t vm);

	libr_init(vm);
}


#endif /* __LIBR_H_ */
