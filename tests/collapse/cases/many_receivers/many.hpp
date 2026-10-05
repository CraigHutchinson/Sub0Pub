#pragma once
/** Case many_receivers: 32 receivers of one type, each bound individually (fan-out scaling). Shared lists so
 *  every variant names the same objects in the same order (c0 first). Not a variant (only .cpp files are). */
#define MANY_COUNT 32U
#define MANY_EACH(X) X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) X(10) X(11) X(12) X(13) X(14) X(15) X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23) X(24) X(25) X(26) X(27) X(28) X(29) X(30) X(31)
#define MANY_ADDRESSES &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9, &c10, &c11, &c12, &c13, &c14, &c15, &c16, &c17, &c18, &c19, &c20, &c21, &c22, &c23, &c24, &c25, &c26, &c27, &c28, &c29, &c30, &c31
#define MANY_GETS c0.get(), c1.get(), c2.get(), c3.get(), c4.get(), c5.get(), c6.get(), c7.get(), c8.get(), c9.get(), c10.get(), c11.get(), c12.get(), c13.get(), c14.get(), c15.get(), c16.get(), c17.get(), c18.get(), c19.get(), c20.get(), c21.get(), c22.get(), c23.get(), c24.get(), c25.get(), c26.get(), c27.get(), c28.get(), c29.get(), c30.get(), c31.get()
#define MANY_TYPES Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller, Controller
