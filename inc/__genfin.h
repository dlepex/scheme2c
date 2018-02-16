/* __genfin.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef __GENFIN_H_
#define __GENFIN_H_

final:
//	oport_write_asci(Out_console_port, "\nResult: >> ");
//	oport_print_datum(vm, Out_console_port, rr);
	UNWIND_RSTK();
	return rr;
}

#endif /* __GENFIN_H_ */
