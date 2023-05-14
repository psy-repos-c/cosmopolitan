#ifndef COSMOPOLITAN_LIBC_TINYMATH_FREEBSD_INTERNAL_H_
#define COSMOPOLITAN_LIBC_TINYMATH_FREEBSD_INTERNAL_H_
#include "libc/assert.h"
#include "libc/complex.h"
#include "libc/math.h"
#include "libc/runtime/fenv.h"
#if !(__ASSEMBLER__ + __LINKER__ + 0)
COSMOPOLITAN_C_START_
// clang-format off

#define	__CONCAT1(x,y)	x ## y
#define	__CONCAT(x,y)	__CONCAT1(x,y)
#define	__STRING(x)	#x
#define	__XSTRING(x)	__STRING(x)

#ifdef __x86_64__

union IEEEl2bits {
	long double	e;
	struct {
		unsigned int	manl	:32;
		unsigned int	manh	:32;
		unsigned int	exp	:15;
		unsigned int	sign	:1;
		unsigned int	junkl	:16;
		unsigned int	junkh	:32;
	} bits;
	struct {
		unsigned long	man	:64;
		unsigned int	expsign	:16;
		unsigned long	junk	:48;
	} xbits;
};

#define	LDBL_NBIT	0x80000000
#define	mask_nbit_l(u)	((u).bits.manh &= ~LDBL_NBIT)

#define	LDBL_MANH_SIZE	32
#define	LDBL_MANL_SIZE	32

#define	LDBL_TO_ARRAY32(u, a) do {			\
	(a)[0] = (uint32_t)(u).bits.manl;		\
	(a)[1] = (uint32_t)(u).bits.manh;		\
} while (0)

#elif defined(__aarch64__)

union IEEEl2bits {
	long double	e;
	struct {
		unsigned long	manl	:64;
		unsigned long	manh	:48;
		unsigned int	exp	:15;
		unsigned int	sign	:1;
	} bits;
	/* TODO andrew: Check the packing here */
	struct {
		unsigned long	manl	:64;
		unsigned long	manh	:48;
		unsigned int	expsign	:16;
	} xbits;
};

#define	LDBL_NBIT	0
#define	LDBL_IMPLICIT_NBIT
#define	mask_nbit_l(u)	((void)0)

#define	LDBL_MANH_SIZE	48
#define	LDBL_MANL_SIZE	64

#define	LDBL_TO_ARRAY32(u, a) do {			\
	(a)[0] = (uint32_t)(u).bits.manl;		\
	(a)[1] = (uint32_t)((u).bits.manl >> 32);	\
	(a)[2] = (uint32_t)(u).bits.manh;		\
	(a)[3] = (uint32_t)((u).bits.manh >> 32);	\
} while(0)

#elif defined(__powerpc64__)

union IEEEl2bits {
	long double	e;
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		unsigned int	manl	:32;
		unsigned int	manh	:20;
		unsigned int	exp	:11;
		unsigned int	sign	:1;
#else	/* _BYTE_ORDER == _LITTLE_ENDIAN */
		unsigned int		sign	:1;
		unsigned int		exp	:11;
		unsigned int		manh	:20;
		unsigned int		manl	:32;
#endif
	} bits;
};

#define	mask_nbit_l(u)	((void)0)
#define	LDBL_IMPLICIT_NBIT
#define	LDBL_NBIT	0

#define	LDBL_MANH_SIZE	20
#define	LDBL_MANL_SIZE	32

#define	LDBL_TO_ARRAY32(u, a) do {			\
	(a)[0] = (uint32_t)(u).bits.manl;		\
	(a)[1] = (uint32_t)(u).bits.manh;		\
} while(0)

#endif /* __x86_64__ */


/*
 * The original fdlibm code used statements like:
 *	n0 = ((*(int*)&one)>>29)^1;		* index of high word *
 *	ix0 = *(n0+(int*)&x);			* high word of x *
 *	ix1 = *((1-n0)+(int*)&x);		* low word of x *
 * to dig two 32 bit words out of the 64 bit IEEE floating point
 * value.  That is non-ANSI, and, moreover, the gcc instruction
 * scheduler gets it wrong.  We instead use the following macros.
 * Unlike the original code, we determine the endianness at compile
 * time, not at run time; I don't see much benefit to selecting
 * endianness at run time.
 */

/*
 * A union which permits us to convert between a double and two 32 bit
 * ints.
 */

#ifdef __arm__
#if defined(__VFP_FP__) || defined(__ARM_EABI__)
#define	IEEE_WORD_ORDER	__BYTE_ORDER__
#else
#define	IEEE_WORD_ORDER	__ORDER_BIG_ENDIAN__
#endif
#else /* __arm__ */
#define	IEEE_WORD_ORDER	__BYTE_ORDER__
#endif

/* A union which permits us to convert between a long double and
   four 32 bit ints.  */

#if IEEE_WORD_ORDER == __ORDER_BIG_ENDIAN__

typedef union
{
  long double value;
  struct {
    uint32_t mswhi;
    uint32_t mswlo;
    uint32_t lswhi;
    uint32_t lswlo;
  } parts32;
  struct {
    uint64_t msw;
    uint64_t lsw;
  } parts64;
} ieee_quad_shape_type;

#endif

#if IEEE_WORD_ORDER == __ORDER_LITTLE_ENDIAN__

typedef union
{
  long double value;
  struct {
    uint32_t lswlo;
    uint32_t lswhi;
    uint32_t mswlo;
    uint32_t mswhi;
  } parts32;
  struct {
    uint64_t lsw;
    uint64_t msw;
  } parts64;
} ieee_quad_shape_type;

#endif

#if IEEE_WORD_ORDER == __ORDER_BIG_ENDIAN__

typedef union
{
  double value;
  struct
  {
    uint32_t msw;
    uint32_t lsw;
  } parts;
  struct
  {
    uint64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

#if IEEE_WORD_ORDER == __ORDER_LITTLE_ENDIAN__

typedef union
{
  double value;
  struct
  {
    uint32_t lsw;
    uint32_t msw;
  } parts;
  struct
  {
    uint64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Get a 64-bit int from a double. */
#define EXTRACT_WORD64(ix,d)					\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix) = ew_u.xparts.w;						\
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)					\
do {								\
  ieee_double_shape_type gl_u;					\
  gl_u.value = (d);						\
  (i) = gl_u.parts.lsw;						\
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)

/* Set a double from a 64-bit int. */
#define INSERT_WORD64(d,ix)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.xparts.w = (ix);						\
  (d) = iw_u.value;						\
} while (0)

/* Set the more significant 32 bits of a double from an int.  */

#define SET_HIGH_WORD(d,v)					\
do {								\
  ieee_double_shape_type sh_u;					\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

/* Set the less significant 32 bits of a double from an int.  */

#define SET_LOW_WORD(d,v)					\
do {								\
  ieee_double_shape_type sl_u;					\
  sl_u.value = (d);						\
  sl_u.parts.lsw = (v);						\
  (d) = sl_u.value;						\
} while (0)

/*
 * A union which permits us to convert between a float and a 32 bit
 * int.
 */

typedef union
{
  float value;
  /* FIXME: Assumes 32 bit int.  */
  unsigned int word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)

/*
 * Get expsign and mantissa as 16 bit and 64 bit ints from an 80 bit long
 * double.
 */

#define	EXTRACT_LDBL80_WORDS(ix0,ix1,d)				\
do {								\
  union IEEEl2bits ew_u;					\
  ew_u.e = (d);							\
  (ix0) = ew_u.xbits.expsign;					\
  (ix1) = ew_u.xbits.man;					\
} while (0)

/*
 * Get expsign and mantissa as one 16 bit and two 64 bit ints from a 128 bit
 * long double.
 */

#define	EXTRACT_LDBL128_WORDS(ix0,ix1,ix2,d)			\
do {								\
  union IEEEl2bits ew_u;					\
  ew_u.e = (d);							\
  (ix0) = ew_u.xbits.expsign;					\
  (ix1) = ew_u.xbits.manh;					\
  (ix2) = ew_u.xbits.manl;					\
} while (0)

/* Get expsign as a 16 bit int from a long double.  */

#define	GET_LDBL_EXPSIGN(i,d)					\
do {								\
  union IEEEl2bits ge_u;					\
  ge_u.e = (d);							\
  (i) = ge_u.xbits.expsign;					\
} while (0)

/*
 * Set an 80 bit long double from a 16 bit int expsign and a 64 bit int
 * mantissa.
 */

#define	INSERT_LDBL80_WORDS(d,ix0,ix1)				\
do {								\
  union IEEEl2bits iw_u;					\
  iw_u.xbits.expsign = (ix0);					\
  iw_u.xbits.man = (ix1);					\
  (d) = iw_u.e;							\
} while (0)

/*
 * Set a 128 bit long double from a 16 bit int expsign and two 64 bit ints
 * comprising the mantissa.
 */

#define	INSERT_LDBL128_WORDS(d,ix0,ix1,ix2)			\
do {								\
  union IEEEl2bits iw_u;					\
  iw_u.xbits.expsign = (ix0);					\
  iw_u.xbits.manh = (ix1);					\
  iw_u.xbits.manl = (ix2);					\
  (d) = iw_u.e;							\
} while (0)

/* Set expsign of a long double from a 16 bit int.  */

#define	SET_LDBL_EXPSIGN(d,v)					\
do {								\
  union IEEEl2bits se_u;					\
  se_u.e = (d);							\
  se_u.xbits.expsign = (v);					\
  (d) = se_u.e;							\
} while (0)

#ifdef __i386__
/* Long double constants are broken on i386. */
#define	LD80C(m, ex, v) {						\
	.xbits.man = __CONCAT(m, ULL),					\
	.xbits.expsign = (0x3fff + (ex)) | ((v) < 0 ? 0x8000 : 0),	\
}
#else
/* The above works on non-i386 too, but we use this to check v. */
#define	LD80C(m, ex, v)	{ .e = (v), }
#endif

#ifdef FLT_EVAL_METHOD
/*
 * Attempt to get strict C99 semantics for assignment with non-C99 compilers.
 */
#if FLT_EVAL_METHOD == 0 || __GNUC__ == 0
#define	STRICT_ASSIGN(type, lval, rval)	((lval) = (rval))
#else
#define	STRICT_ASSIGN(type, lval, rval) do {	\
	volatile type __lval;			\
						\
	if (sizeof(type) >= sizeof(long double))	\
		(lval) = (rval);		\
	else {					\
		__lval = (rval);		\
		(lval) = __lval;		\
	}					\
} while (0)
#endif
#endif /* FLT_EVAL_METHOD */

/* Support switching the mode to FP_PE if necessary. */
#if defined(__i386__) && !defined(NO_FPSETPREC)
#define	ENTERI() ENTERIT(long double)
#define	ENTERIT(returntype)			\
	returntype __retval;			\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNI(x) do {				\
	__retval = (x);				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	RETURNF(__retval);			\
} while (0)
#define	ENTERV()				\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNV() do {				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	return;			\
} while (0)
#else
#define	ENTERI()
#define	ENTERIT(x)
#define	RETURNI(x)	RETURNF(x)
#define	ENTERV()
#define	RETURNV()	return
#endif

/* Default return statement if hack*_t() is not used. */
#define      RETURNF(v)      return (v)

/*
 * 2sum gives the same result as 2sumF without requiring |a| >= |b| or
 * a == 0, but is slower.
 */
#define	_2sum(a, b) do {	\
	__typeof(a) __s, __w;	\
				\
	__w = (a) + (b);	\
	__s = __w - (a);	\
	(b) = ((a) - (__w - __s)) + ((b) - __s); \
	(a) = __w;		\
} while (0)

/*
 * 2sumF algorithm.
 *
 * "Normalize" the terms in the infinite-precision expression a + b for
 * the sum of 2 floating point values so that b is as small as possible
 * relative to 'a'.  (The resulting 'a' is the value of the expression in
 * the same precision as 'a' and the resulting b is the rounding error.)
 * |a| must be >= |b| or 0, b's type must be no larger than 'a's type, and
 * exponent overflow or underflow must not occur.  This uses a Theorem of
 * Dekker (1971).  See Knuth (1981) 4.2.2 Theorem C.  The name "TwoSum"
 * is apparently due to Skewchuk (1997).
 *
 * For this to always work, assignment of a + b to 'a' must not retain any
 * extra precision in a + b.  This is required by C standards but broken
 * in many compilers.  The brokenness cannot be worked around using
 * STRICT_ASSIGN() like we do elsewhere, since the efficiency of this
 * algorithm would be destroyed by non-null strict assignments.  (The
 * compilers are correct to be broken -- the efficiency of all floating
 * point code calculations would be destroyed similarly if they forced the
 * conversions.)
 *
 * Fortunately, a case that works well can usually be arranged by building
 * any extra precision into the type of 'a' -- 'a' should have type float_t,
 * double_t or long double.  b's type should be no larger than 'a's type.
 * Callers should use these types with scopes as large as possible, to
 * reduce their own extra-precision and efficiciency problems.  In
 * particular, they shouldn't convert back and forth just to call here.
 */
#ifdef DEBUG
#define	_2sumF(a, b) do {				\
	__typeof(a) __w;				\
	volatile __typeof(a) __ia, __ib, __r, __vw;	\
							\
	__ia = (a);					\
	__ib = (b);					\
	assert(__ia == 0 || fabsl(__ia) >= fabsl(__ib));	\
							\
	__w = (a) + (b);				\
	(b) = ((a) - __w) + (b);			\
	(a) = __w;					\
							\
	/* The next 2 assertions are weak if (a) is already long double. */ \
	assert((long double)__ia + __ib == (long double)(a) + (b));	\
	__vw = __ia + __ib;				\
	__r = __ia - __vw;				\
	__r += __ib;					\
	assert(__vw == (a) && __r == (b));		\
} while (0)
#else /* !DEBUG */
#define	_2sumF(a, b) do {	\
	__typeof(a) __w;	\
				\
	__w = (a) + (b);	\
	(b) = ((a) - __w) + (b); \
	(a) = __w;		\
} while (0)
#endif /* DEBUG */

/*
 * Set x += c, where x is represented in extra precision as a + b.
 * x must be sufficiently normalized and sufficiently larger than c,
 * and the result is then sufficiently normalized.
 *
 * The details of ordering are that |a| must be >= |c| (so that (a, c)
 * can be normalized without extra work to swap 'a' with c).  The details of
 * the normalization are that b must be small relative to the normalized 'a'.
 * Normalization of (a, c) makes the normalized c tiny relative to the
 * normalized a, so b remains small relative to 'a' in the result.  However,
 * b need not ever be tiny relative to 'a'.  For example, b might be about
 * 2**20 times smaller than 'a' to give about 20 extra bits of precision.
 * That is usually enough, and adding c (which by normalization is about
 * 2**53 times smaller than a) cannot change b significantly.  However,
 * cancellation of 'a' with c in normalization of (a, c) may reduce 'a'
 * significantly relative to b.  The caller must ensure that significant
 * cancellation doesn't occur, either by having c of the same sign as 'a',
 * or by having |c| a few percent smaller than |a|.  Pre-normalization of
 * (a, b) may help.
 *
 * This is a variant of an algorithm of Kahan (see Knuth (1981) 4.2.2
 * exercise 19).  We gain considerable efficiency by requiring the terms to
 * be sufficiently normalized and sufficiently increasing.
 */
#define	_3sumF(a, b, c) do {	\
	__typeof(a) __tmp;	\
				\
	__tmp = (c);		\
	_2sumF(__tmp, (a));	\
	(b) += (a);		\
	(a) = __tmp;		\
} while (0)

/*
 * Common routine to process the arguments to nan(), nanf(), and nanl().
 */
void _scan_nan(uint32_t *__words, int __num_words, const char *__s);

/*
 * Mix 0, 1 or 2 NaNs.  First add 0 to each arg.  This normally just turns
 * signaling NaNs into quiet NaNs by setting a quiet bit.  We do this
 * because we want to never return a signaling NaN, and also because we
 * don't want the quiet bit to affect the result.  Then mix the converted
 * args using the specified operation.
 *
 * When one arg is NaN, the result is typically that arg quieted.  When both
 * args are NaNs, the result is typically the quietening of the arg whose
 * mantissa is largest after quietening.  When neither arg is NaN, the
 * result may be NaN because it is indeterminate, or finite for subsequent
 * construction of a NaN as the indeterminate 0.0L/0.0L.
 *
 * Technical complications: the result in bits after rounding to the final
 * precision might depend on the runtime precision and/or on compiler
 * optimizations, especially when different register sets are used for
 * different precisions.  Try to make the result not depend on at least the
 * runtime precision by always doing the main mixing step in long double
 * precision.  Try to reduce dependencies on optimizations by adding the
 * the 0's in different precisions (unless everything is in long double
 * precision).
 */
#define	nan_mix(x, y)		(nan_mix_op((x), (y), +))
#define	nan_mix_op(x, y, op)	(((x) + 0.0L) op ((y) + 0))

#ifdef _COMPLEX_H

/*
 * C99 specifies that complex numbers have the same representation as
 * an array of two elements, where the first element is the real part
 * and the second element is the imaginary part.
 */
typedef union {
	float complex f;
	float a[2];
} float_complex;
typedef union {
	double complex f;
	double a[2];
} double_complex;
typedef union {
	long double complex f;
	long double a[2];
} long_double_complex;
#define	REALPART(z)	((z).a[0])
#define	IMAGPART(z)	((z).a[1])

/*
 * Inline functions that can be used to construct complex values.
 *
 * The C99 standard intends x+I*y to be used for this, but x+I*y is
 * currently unusable in general since gcc introduces many overflow,
 * underflow, sign and efficiency bugs by rewriting I*y as
 * (0.0+I)*(y+0.0*I) and laboriously computing the full complex product.
 * In particular, I*Inf is corrupted to NaN+I*Inf, and I*-0 is corrupted
 * to -0.0+I*0.0.
 *
 * The C11 standard introduced the macros CMPLX(), CMPLXF() and CMPLXL()
 * to construct complex values.  Compilers that conform to the C99
 * standard require the following functions to avoid the above issues.
 */

#ifndef CMPLXF
static __inline float complex
CMPLXF(float x, float y)
{
	float_complex z;

	REALPART(z) = x;
	IMAGPART(z) = y;
	return (z.f);
}
#endif

#ifndef CMPLX
static __inline double complex
CMPLX(double x, double y)
{
	double_complex z;

	REALPART(z) = x;
	IMAGPART(z) = y;
	return (z.f);
}
#endif

#ifndef CMPLXL
static __inline long double complex
CMPLXL(long double x, long double y)
{
	long_double_complex z;

	REALPART(z) = x;
	IMAGPART(z) = y;
	return (z.f);
}
#endif

#endif /* _COMPLEX_H */

/*
 * The rnint() family rounds to the nearest integer for a restricted range
 * range of args (up to about 2**MANT_DIG).  We assume that the current
 * rounding mode is FE_TONEAREST so that this can be done efficiently.
 * Extra precision causes more problems in practice, and we only centralize
 * this here to reduce those problems, and have not solved the efficiency
 * problems.  The exp2() family uses a more delicate version of this that
 * requires extracting bits from the intermediate value, so it is not
 * centralized here and should copy any solution of the efficiency problems.
 */

static inline double
rnint(double_t x)
{
	/*
	 * This casts to double to kill any extra precision.  This depends
	 * on the cast being applied to a double_t to avoid compiler bugs
	 * (this is a cleaner version of STRICT_ASSIGN()).  This is
	 * inefficient if there actually is extra precision, but is hard
	 * to improve on.  We use double_t in the API to minimise conversions
	 * for just calling here.  Note that we cannot easily change the
	 * magic number to the one that works directly with double_t, since
	 * the rounding precision is variable at runtime on x86 so the
	 * magic number would need to be variable.  Assuming that the
	 * rounding precision is always the default is too fragile.  This
	 * and many other complications will move when the default is
	 * changed to FP_PE.
	 */
	return ((double)(x + 0x1.8p52) - 0x1.8p52);
}

static inline float
rnintf(float_t x)
{
	/*
	 * As for rnint(), except we could just call that to handle the
	 * extra precision case, usually without losing efficiency.
	 */
	return ((float)(x + 0x1.8p23F) - 0x1.8p23F);
}

#ifdef LDBL_MANT_DIG
/*
 * The complications for extra precision are smaller for rnintl() since it
 * can safely assume that the rounding precision has been increased from
 * its default to FP_PE on x86.  We don't exploit that here to get small
 * optimizations from limiting the rangle to double.  We just need it for
 * the magic number to work with long doubles.  ld128 callers should use
 * rnint() instead of this if possible.  ld80 callers should prefer
 * rnintl() since for amd64 this avoids swapping the register set, while
 * for i386 it makes no difference (assuming FP_PE), and for other arches
 * it makes little difference.
 */
static inline long double
rnintl(long double x)
{
	return (x + __CONCAT(0x1.8p, LDBL_MANT_DIG) / 2 -
	    __CONCAT(0x1.8p, LDBL_MANT_DIG) / 2);
}
#endif /* LDBL_MANT_DIG */

/*
 * irint() and i64rint() give the same result as casting to their integer
 * return type provided their arg is a floating point integer.  They can
 * sometimes be more efficient because no rounding is required.
 */
#if defined(amd64) || defined(__i386__)
#define	irint(x)						\
    (sizeof(x) == sizeof(float) &&				\
    sizeof(float_t) == sizeof(long double) ? irintf(x) :	\
    sizeof(x) == sizeof(double) &&				\
    sizeof(double_t) == sizeof(long double) ? irintd(x) :	\
    sizeof(x) == sizeof(long double) ? irintl(x) : (int)(x))
#else
#define	irint(x)	((int)(x))
#endif

#define	i64rint(x)	((int64_t)(x))	/* only needed for ld128 so not opt. */

#if defined(__i386__)
static __inline int
irintf(float x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}

static __inline int
irintd(double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

#if defined(__amd64__) || defined(__i386__)
static __inline int
irintl(long double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

#ifdef DEBUG
#if defined(__amd64__) || defined(__i386__)
#define	breakpoint()	asm("int $3")
#else
#define	breakpoint()	raise(SIGTRAP)
#endif
#endif

/* Write a pari script to test things externally. */
#ifdef DOPRINT

#ifndef DOPRINT_SWIZZLE
#define	DOPRINT_SWIZZLE		0
#endif

#ifdef DOPRINT_LD80

#define	DOPRINT_START(xp) do {						\
	uint64_t __lx;							\
	uint16_t __hx;							\
									\
	/* Hack to give more-problematic args. */			\
	EXTRACT_LDBL80_WORDS(__hx, __lx, *xp);				\
	__lx ^= DOPRINT_SWIZZLE;					\
	INSERT_LDBL80_WORDS(*xp, __hx, __lx);				\
	printf("x = %.21Lg; ", (long double)*xp);			\
} while (0)
#define	DOPRINT_END1(v)							\
	printf("y = %.21Lg; z = 0; show(x, y, z);\n", (long double)(v))
#define	DOPRINT_END2(hi, lo)						\
	printf("y = %.21Lg; z = %.21Lg; show(x, y, z);\n",		\
	    (long double)(hi), (long double)(lo))

#elif defined(DOPRINT_D64)

#define	DOPRINT_START(xp) do {						\
	uint32_t __hx, __lx;						\
									\
	EXTRACT_WORDS(__hx, __lx, *xp);					\
	__lx ^= DOPRINT_SWIZZLE;					\
	INSERT_WORDS(*xp, __hx, __lx);					\
	printf("x = %.21Lg; ", (long double)*xp);			\
} while (0)
#define	DOPRINT_END1(v)							\
	printf("y = %.21Lg; z = 0; show(x, y, z);\n", (long double)(v))
#define	DOPRINT_END2(hi, lo)						\
	printf("y = %.21Lg; z = %.21Lg; show(x, y, z);\n",		\
	    (long double)(hi), (long double)(lo))

#elif defined(DOPRINT_F32)

#define	DOPRINT_START(xp) do {						\
	uint32_t __hx;							\
									\
	GET_FLOAT_WORD(__hx, *xp);					\
	__hx ^= DOPRINT_SWIZZLE;					\
	SET_FLOAT_WORD(*xp, __hx);					\
	printf("x = %.21Lg; ", (long double)*xp);			\
} while (0)
#define	DOPRINT_END1(v)							\
	printf("y = %.21Lg; z = 0; show(x, y, z);\n", (long double)(v))
#define	DOPRINT_END2(hi, lo)						\
	printf("y = %.21Lg; z = %.21Lg; show(x, y, z);\n",		\
	    (long double)(hi), (long double)(lo))

#else /* !DOPRINT_LD80 && !DOPRINT_D64 (LD128 only) */

#ifndef DOPRINT_SWIZZLE_HIGH
#define	DOPRINT_SWIZZLE_HIGH	0
#endif

#define	DOPRINT_START(xp) do {						\
	uint64_t __lx, __llx;						\
	uint16_t __hx;							\
									\
	EXTRACT_LDBL128_WORDS(__hx, __lx, __llx, *xp);			\
	__llx ^= DOPRINT_SWIZZLE;					\
	__lx ^= DOPRINT_SWIZZLE_HIGH;					\
	INSERT_LDBL128_WORDS(*xp, __hx, __lx, __llx);			\
	printf("x = %.36Lg; ", (long double)*xp);					\
} while (0)
#define	DOPRINT_END1(v)							\
	printf("y = %.36Lg; z = 0; show(x, y, z);\n", (long double)(v))
#define	DOPRINT_END2(hi, lo)						\
	printf("y = %.36Lg; z = %.36Lg; show(x, y, z);\n",		\
	    (long double)(hi), (long double)(lo))

#endif /* DOPRINT_LD80 */

#else /* !DOPRINT */
#define	DOPRINT_START(xp)
#define	DOPRINT_END1(v)
#define	DOPRINT_END2(hi, lo)
#endif /* DOPRINT */

#define	RETURNP(x) do {			\
	DOPRINT_END1(x);		\
	RETURNF(x);			\
} while (0)
#define	RETURNPI(x) do {		\
	DOPRINT_END1(x);		\
	RETURNI(x);			\
} while (0)
#define	RETURN2P(x, y) do {		\
	DOPRINT_END2((x), (y));		\
	RETURNF((x) + (y));		\
} while (0)
#define	RETURN2PI(x, y) do {		\
	DOPRINT_END2((x), (y));		\
	RETURNI((x) + (y));		\
} while (0)
#define	RETURNSP(rp) do {		\
	if (!(rp)->lo_set)		\
		RETURNP((rp)->hi);	\
	RETURN2P((rp)->hi, (rp)->lo);	\
} while (0)
#define	RETURNSPI(rp) do {		\
	if (!(rp)->lo_set)		\
		RETURNPI((rp)->hi);	\
	RETURN2PI((rp)->hi, (rp)->lo);	\
} while (0)
#define	SUM2P(x, y) ({			\
	const __typeof (x) __x = (x);	\
	const __typeof (y) __y = (y);	\
					\
	DOPRINT_END2(__x, __y);		\
	__x + __y;			\
})

/*
 * ieee style elementary functions
 *
 * We rename functions here to improve other sources' diffability
 * against fdlibm.
 */
#define	__ieee754_sqrt	sqrt
#define	__ieee754_acos	acos
#define	__ieee754_acosh	acosh
#define	__ieee754_log	log
#define	__ieee754_log2	log2
#define	__ieee754_atanh	atanh
#define	__ieee754_asin	asin
#define	__ieee754_atan2	atan2
#define	__ieee754_exp	exp
#define	__ieee754_cosh	cosh
#define	__ieee754_fmod	fmod
#define	__ieee754_pow	pow
#define	__ieee754_lgamma lgamma
#define	__ieee754_gamma	gamma
#define	__ieee754_lgamma_r lgamma_r
#define	__ieee754_gamma_r gamma_r
#define	__ieee754_log10	log10
#define	__ieee754_sinh	sinh
#define	__ieee754_hypot	hypot
#define	__ieee754_j0	j0
#define	__ieee754_j1	j1
#define	__ieee754_y0	y0
#define	__ieee754_y1	y1
#define	__ieee754_jn	jn
#define	__ieee754_yn	yn
#define	__ieee754_remainder remainder
#define	__ieee754_scalb	scalb
#define	__ieee754_sqrtf	sqrtf
#define	__ieee754_acosf	acosf
#define	__ieee754_acoshf acoshf
#define	__ieee754_logf	logf
#define	__ieee754_atanhf atanhf
#define	__ieee754_asinf	asinf
#define	__ieee754_atan2f atan2f
#define	__ieee754_expf	expf
#define	__ieee754_coshf	coshf
#define	__ieee754_fmodf	fmodf
#define	__ieee754_powf	powf
#define	__ieee754_lgammaf lgammaf
#define	__ieee754_gammaf gammaf
#define	__ieee754_lgammaf_r lgammaf_r
#define	__ieee754_gammaf_r gammaf_r
#define	__ieee754_log10f log10f
#define	__ieee754_log2f log2f
#define	__ieee754_sinhf	sinhf
#define	__ieee754_hypotf hypotf
#define	__ieee754_j0f	j0f
#define	__ieee754_j1f	j1f
#define	__ieee754_y0f	y0f
#define	__ieee754_y1f	y1f
#define	__ieee754_jnf	jnf
#define	__ieee754_ynf	ynf
#define	__ieee754_remainderf remainderf
#define	__ieee754_scalbf scalbf

/* fdlibm kernel function */
int	__kernel_rem_pio2(double*,double*,int,int,int);

/* double precision kernel functions */
#ifndef INLINE_REM_PIO2
int	__ieee754_rem_pio2(double,double*);
#endif
double	__kernel_sin(double,double,int);
double	__kernel_cos(double,double);
double	__kernel_tan(double,double,int);
double	__ldexp_exp(double,int);
#ifdef _COMPLEX_H
double complex __ldexp_cexp(double complex,int);
#endif

/* float precision kernel functions */
#ifndef INLINE_REM_PIO2F
int	__ieee754_rem_pio2f(float,double*);
#endif
#ifndef INLINE_KERNEL_SINDF
float	__kernel_sindf(double);
#endif
#ifndef INLINE_KERNEL_COSDF
float	__kernel_cosdf(double);
#endif
#ifndef INLINE_KERNEL_TANDF
float	__kernel_tandf(double,int);
#endif
float	__ldexp_expf(float,int);
#ifdef _COMPLEX_H
float complex __ldexp_cexpf(float complex,int);
#endif

/* long double precision kernel functions */
long double __kernel_sinl(long double, long double, int);
long double __kernel_cosl(long double, long double);
long double __kernel_tanl(long double, long double, int);

/*
 * ld128 version of k_expl.h.  See ../ld80/s_expl.c for most comments.
 *
 * See ../src/e_exp.c and ../src/k_exp.h for precision-independent comments
 * about the secondary kernels.
 */

#define	INTERVALS	128
#define	LOG2_INTERVALS	7
#define	BIAS	(LDBL_MAX_EXP - 1)

static const double
/*
 * ln2/INTERVALS = L1+L2 (hi+lo decomposition for multiplication).  L1 must
 * have at least 22 (= log2(|LDBL_MIN_EXP-extras|) + log2(INTERVALS)) lowest
 * bits zero so that multiplication of it by n is exact.
 */
INV_L = 1.8466496523378731e+2,		/*  0x171547652b82fe.0p-45 */
L2 = -1.0253670638894731e-29;		/* -0x1.9ff0342542fc3p-97 */
static const long double
/* 0x1.62e42fefa39ef35793c768000000p-8 */
L1 =  5.41521234812457272982212595914567508e-3L;

/*
 * XXX values in hex in comments have been lost (or were never present)
 * from here.
 */
static const long double
/*
 * Domain [-0.002708, 0.002708], range ~[-2.4021e-38, 2.4234e-38]:
 * |exp(x) - p(x)| < 2**-124.9
 * (0.002708 is ln2/(2*INTERVALS) rounded up a little).
 *
 * XXX the coeffs aren't very carefully rounded, and I get 3.6 more bits.
 */
A2  =  0.5,
A3  =  1.66666666666666666666666666651085500e-1L,
A4  =  4.16666666666666666666666666425885320e-2L,
A5  =  8.33333333333333333334522877160175842e-3L,
A6  =  1.38888888888888888889971139751596836e-3L;

static const double
A7  =  1.9841269841269470e-4,		/*  0x1.a01a01a019f91p-13 */
A8  =  2.4801587301585286e-5,		/*  0x1.71de3ec75a967p-19 */
A9  =  2.7557324277411235e-6,		/*  0x1.71de3ec75a967p-19 */
A10 =  2.7557333722375069e-7;		/*  0x1.27e505ab56259p-22 */

static const struct {
	/*
	 * hi must be rounded to at most 106 bits so that multiplication
	 * by r1 in expm1l() is exact, but it is rounded to 88 bits due to
	 * historical accidents.
	 *
	 * XXX it is wasteful to use long double for both hi and lo.  ld128
	 * exp2l() uses only float for lo (in a very differently organized
	 * table; ld80 exp2l() is different again.  It uses 2 doubles in a
	 * table organized like this one.  1 double and 1 float would
	 * suffice).  There are different packing/locality/alignment/caching
	 * problems with these methods.
	 *
	 * XXX C's bad %a format makes the bits unreadable.  They happen
	 * to all line up for the hi values 1 before the point and 88
	 * in 22 nybbles, but for the low values the nybbles are shifted
	 * randomly.
	 */
	long double	hi;
	long double	lo;
} tbl[INTERVALS] = {
	{0x1p0L, 0x0p0L},
	{0x1.0163da9fb33356d84a66aep0L, 0x3.36dcdfa4003ec04c360be2404078p-92L},
	{0x1.02c9a3e778060ee6f7cacap0L, 0x4.f7a29bde93d70a2cabc5cb89ba10p-92L},
	{0x1.04315e86e7f84bd738f9a2p0L, 0xd.a47e6ed040bb4bfc05af6455e9b8p-96L},
	{0x1.059b0d31585743ae7c548ep0L, 0xb.68ca417fe53e3495f7df4baf84a0p-92L},
	{0x1.0706b29ddf6ddc6dc403a8p0L, 0x1.d87b27ed07cb8b092ac75e311753p-88L},
	{0x1.0874518759bc808c35f25cp0L, 0x1.9427fa2b041b2d6829d8993a0d01p-88L},
	{0x1.09e3ecac6f3834521e060cp0L, 0x5.84d6b74ba2e023da730e7fccb758p-92L},
	{0x1.0b5586cf9890f6298b92b6p0L, 0x1.1842a98364291408b3ceb0a2a2bbp-88L},
	{0x1.0cc922b7247f7407b705b8p0L, 0x9.3dc5e8aac564e6fe2ef1d431fd98p-92L},
	{0x1.0e3ec32d3d1a2020742e4ep0L, 0x1.8af6a552ac4b358b1129e9f966a4p-88L},
	{0x1.0fb66affed31af232091dcp0L, 0x1.8a1426514e0b627bda694a400a27p-88L},
	{0x1.11301d0125b50a4ebbf1aep0L, 0xd.9318ceac5cc47ab166ee57427178p-92L},
	{0x1.12abdc06c31cbfb92bad32p0L, 0x4.d68e2f7270bdf7cedf94eb1cb818p-92L},
	{0x1.1429aaea92ddfb34101942p0L, 0x1.b2586d01844b389bea7aedd221d4p-88L},
	{0x1.15a98c8a58e512480d573cp0L, 0x1.d5613bf92a2b618ee31b376c2689p-88L},
	{0x1.172b83c7d517adcdf7c8c4p0L, 0x1.0eb14a792035509ff7d758693f24p-88L},
	{0x1.18af9388c8de9bbbf70b9ap0L, 0x3.c2505c97c0102e5f1211941d2840p-92L},
	{0x1.1a35beb6fcb753cb698f68p0L, 0x1.2d1c835a6c30724d5cfae31b84e5p-88L},
	{0x1.1bbe084045cd39ab1e72b4p0L, 0x4.27e35f9acb57e473915519a1b448p-92L},
	{0x1.1d4873168b9aa7805b8028p0L, 0x9.90f07a98b42206e46166cf051d70p-92L},
	{0x1.1ed5022fcd91cb8819ff60p0L, 0x1.121d1e504d36c47474c9b7de6067p-88L},
	{0x1.2063b88628cd63b8eeb028p0L, 0x1.50929d0fc487d21c2b84004264dep-88L},
	{0x1.21f49917ddc962552fd292p0L, 0x9.4bdb4b61ea62477caa1dce823ba0p-92L},
	{0x1.2387a6e75623866c1fadb0p0L, 0x1.c15cb593b0328566902df69e4de2p-88L},
	{0x1.251ce4fb2a63f3582ab7dep0L, 0x9.e94811a9c8afdcf796934bc652d0p-92L},
	{0x1.26b4565e27cdd257a67328p0L, 0x1.d3b249dce4e9186ddd5ff44e6b08p-92L},
	{0x1.284dfe1f5638096cf15cf0p0L, 0x3.ca0967fdaa2e52d7c8106f2e262cp-92L},
	{0x1.29e9df51fdee12c25d15f4p0L, 0x1.a24aa3bca890ac08d203fed80a07p-88L},
	{0x1.2b87fd0dad98ffddea4652p0L, 0x1.8fcab88442fdc3cb6de4519165edp-88L},
	{0x1.2d285a6e4030b40091d536p0L, 0xd.075384589c1cd1b3e4018a6b1348p-92L},
	{0x1.2ecafa93e2f5611ca0f45cp0L, 0x1.523833af611bdcda253c554cf278p-88L},
	{0x1.306fe0a31b7152de8d5a46p0L, 0x3.05c85edecbc27343629f502f1af2p-92L},
	{0x1.32170fc4cd8313539cf1c2p0L, 0x1.008f86dde3220ae17a005b6412bep-88L},
	{0x1.33c08b26416ff4c9c8610cp0L, 0x1.96696bf95d1593039539d94d662bp-88L},
	{0x1.356c55f929ff0c94623476p0L, 0x3.73af38d6d8d6f9506c9bbc93cbc0p-92L},
	{0x1.371a7373aa9caa7145502ep0L, 0x1.4547987e3e12516bf9c699be432fp-88L},
	{0x1.38cae6d05d86585a9cb0d8p0L, 0x1.bed0c853bd30a02790931eb2e8f0p-88L},
	{0x1.3a7db34e59ff6ea1bc9298p0L, 0x1.e0a1d336163fe2f852ceeb134067p-88L},
	{0x1.3c32dc313a8e484001f228p0L, 0xb.58f3775e06ab66353001fae9fca0p-92L},
	{0x1.3dea64c12342235b41223ep0L, 0x1.3d773fba2cb82b8244267c54443fp-92L},
	{0x1.3fa4504ac801ba0bf701aap0L, 0x4.1832fb8c1c8dbdff2c49909e6c60p-92L},
	{0x1.4160a21f72e29f84325b8ep0L, 0x1.3db61fb352f0540e6ba05634413ep-88L},
	{0x1.431f5d950a896dc7044394p0L, 0x1.0ccec81e24b0caff7581ef4127f7p-92L},
	{0x1.44e086061892d03136f408p0L, 0x1.df019fbd4f3b48709b78591d5cb5p-88L},
	{0x1.46a41ed1d005772512f458p0L, 0x1.229d97df404ff21f39c1b594d3a8p-88L},
	{0x1.486a2b5c13cd013c1a3b68p0L, 0x1.062f03c3dd75ce8757f780e6ec99p-88L},
	{0x1.4a32af0d7d3de672d8bcf4p0L, 0x6.f9586461db1d878b1d148bd3ccb8p-92L},
	{0x1.4bfdad5362a271d4397afep0L, 0xc.42e20e0363ba2e159c579f82e4b0p-92L},
	{0x1.4dcb299fddd0d63b36ef1ap0L, 0x9.e0cc484b25a5566d0bd5f58ad238p-92L},
	{0x1.4f9b2769d2ca6ad33d8b68p0L, 0x1.aa073ee55e028497a329a7333dbap-88L},
	{0x1.516daa2cf6641c112f52c8p0L, 0x4.d822190e718226177d7608d20038p-92L},
	{0x1.5342b569d4f81df0a83c48p0L, 0x1.d86a63f4e672a3e429805b049465p-88L},
	{0x1.551a4ca5d920ec52ec6202p0L, 0x4.34ca672645dc6c124d6619a87574p-92L},
	{0x1.56f4736b527da66ecb0046p0L, 0x1.64eb3c00f2f5ab3d801d7cc7272dp-88L},
	{0x1.58d12d497c7fd252bc2b72p0L, 0x1.43bcf2ec936a970d9cc266f0072fp-88L},
	{0x1.5ab07dd48542958c930150p0L, 0x1.91eb345d88d7c81280e069fbdb63p-88L},
	{0x1.5c9268a5946b701c4b1b80p0L, 0x1.6986a203d84e6a4a92f179e71889p-88L},
	{0x1.5e76f15ad21486e9be4c20p0L, 0x3.99766a06548a05829e853bdb2b52p-92L},
	{0x1.605e1b976dc08b076f592ap0L, 0x4.86e3b34ead1b4769df867b9c89ccp-92L},
	{0x1.6247eb03a5584b1f0fa06ep0L, 0x1.d2da42bb1ceaf9f732275b8aef30p-88L},
	{0x1.6434634ccc31fc76f8714cp0L, 0x4.ed9a4e41000307103a18cf7a6e08p-92L},
	{0x1.66238825522249127d9e28p0L, 0x1.b8f314a337f4dc0a3adf1787ff74p-88L},
	{0x1.68155d44ca973081c57226p0L, 0x1.b9f32706bfe4e627d809a85dcc66p-88L},
	{0x1.6a09e667f3bcc908b2fb12p0L, 0x1.66ea957d3e3adec17512775099dap-88L},
	{0x1.6c012750bdabeed76a9980p0L, 0xf.4f33fdeb8b0ecd831106f57b3d00p-96L},
	{0x1.6dfb23c651a2ef220e2cbep0L, 0x1.bbaa834b3f11577ceefbe6c1c411p-92L},
	{0x1.6ff7df9519483cf87e1b4ep0L, 0x1.3e213bff9b702d5aa477c12523cep-88L},
	{0x1.71f75e8ec5f73dd2370f2ep0L, 0xf.0acd6cb434b562d9e8a20adda648p-92L},
	{0x1.73f9a48a58173bd5c9a4e6p0L, 0x8.ab1182ae217f3a7681759553e840p-92L},
	{0x1.75feb564267c8bf6e9aa32p0L, 0x1.a48b27071805e61a17b954a2dad8p-88L},
	{0x1.780694fde5d3f619ae0280p0L, 0x8.58b2bb2bdcf86cd08e35fb04c0f0p-92L},
	{0x1.7a11473eb0186d7d51023ep0L, 0x1.6cda1f5ef42b66977960531e821bp-88L},
	{0x1.7c1ed0130c1327c4933444p0L, 0x1.937562b2dc933d44fc828efd4c9cp-88L},
	{0x1.7e2f336cf4e62105d02ba0p0L, 0x1.5797e170a1427f8fcdf5f3906108p-88L},
	{0x1.80427543e1a11b60de6764p0L, 0x9.a354ea706b8e4d8b718a672bf7c8p-92L},
	{0x1.82589994cce128acf88afap0L, 0xb.34a010f6ad65cbbac0f532d39be0p-92L},
	{0x1.8471a4623c7acce52f6b96p0L, 0x1.c64095370f51f48817914dd78665p-88L},
	{0x1.868d99b4492ec80e41d90ap0L, 0xc.251707484d73f136fb5779656b70p-92L},
	{0x1.88ac7d98a669966530bcdep0L, 0x1.2d4e9d61283ef385de170ab20f96p-88L},
	{0x1.8ace5422aa0db5ba7c55a0p0L, 0x1.92c9bb3e6ed61f2733304a346d8fp-88L},
	{0x1.8cf3216b5448bef2aa1cd0p0L, 0x1.61c55d84a9848f8c453b3ca8c946p-88L},
	{0x1.8f1ae991577362b982745cp0L, 0x7.2ed804efc9b4ae1458ae946099d4p-92L},
	{0x1.9145b0b91ffc588a61b468p0L, 0x1.f6b70e01c2a90229a4c4309ea719p-88L},
	{0x1.93737b0cdc5e4f4501c3f2p0L, 0x5.40a22d2fc4af581b63e8326efe9cp-92L},
	{0x1.95a44cbc8520ee9b483694p0L, 0x1.a0fc6f7c7d61b2b3a22a0eab2cadp-88L},
	{0x1.97d829fde4e4f8b9e920f8p0L, 0x1.1e8bd7edb9d7144b6f6818084cc7p-88L},
	{0x1.9a0f170ca07b9ba3109b8cp0L, 0x4.6737beb19e1eada6825d3c557428p-92L},
	{0x1.9c49182a3f0901c7c46b06p0L, 0x1.1f2be58ddade50c217186c90b457p-88L},
	{0x1.9e86319e323231824ca78ep0L, 0x6.4c6e010f92c082bbadfaf605cfd4p-92L},
	{0x1.a0c667b5de564b29ada8b8p0L, 0xc.ab349aa0422a8da7d4512edac548p-92L},
	{0x1.a309bec4a2d3358c171f76p0L, 0x1.0daad547fa22c26d168ea762d854p-88L},
	{0x1.a5503b23e255c8b424491cp0L, 0xa.f87bc8050a405381703ef7caff50p-92L},
	{0x1.a799e1330b3586f2dfb2b0p0L, 0x1.58f1a98796ce8908ae852236ca94p-88L},
	{0x1.a9e6b5579fdbf43eb243bcp0L, 0x1.ff4c4c58b571cf465caf07b4b9f5p-88L},
	{0x1.ac36bbfd3f379c0db966a2p0L, 0x1.1265fc73e480712d20f8597a8e7bp-88L},
	{0x1.ae89f995ad3ad5e8734d16p0L, 0x1.73205a7fbc3ae675ea440b162d6cp-88L},
	{0x1.b0e07298db66590842acdep0L, 0x1.c6f6ca0e5dcae2aafffa7a0554cbp-88L},
	{0x1.b33a2b84f15faf6bfd0e7ap0L, 0x1.d947c2575781dbb49b1237c87b6ep-88L},
	{0x1.b59728de559398e3881110p0L, 0x1.64873c7171fefc410416be0a6525p-88L},
	{0x1.b7f76f2fb5e46eaa7b081ap0L, 0xb.53c5354c8903c356e4b625aacc28p-92L},
	{0x1.ba5b030a10649840cb3c6ap0L, 0xf.5b47f297203757e1cc6eadc8bad0p-92L},
	{0x1.bcc1e904bc1d2247ba0f44p0L, 0x1.b3d08cd0b20287092bd59be4ad98p-88L},
	{0x1.bf2c25bd71e088408d7024p0L, 0x1.18e3449fa073b356766dfb568ff4p-88L},
	{0x1.c199bdd85529c2220cb12ap0L, 0x9.1ba6679444964a36661240043970p-96L},
	{0x1.c40ab5fffd07a6d14df820p0L, 0xf.1828a5366fd387a7bdd54cdf7300p-92L},
	{0x1.c67f12e57d14b4a2137fd2p0L, 0xf.2b301dd9e6b151a6d1f9d5d5f520p-96L},
	{0x1.c8f6d9406e7b511acbc488p0L, 0x5.c442ddb55820171f319d9e5076a8p-96L},
	{0x1.cb720dcef90691503cbd1ep0L, 0x9.49db761d9559ac0cb6dd3ed599e0p-92L},
	{0x1.cdf0b555dc3f9c44f8958ep0L, 0x1.ac51be515f8c58bdfb6f5740a3a4p-88L},
	{0x1.d072d4a07897b8d0f22f20p0L, 0x1.a158e18fbbfc625f09f4cca40874p-88L},
	{0x1.d2f87080d89f18ade12398p0L, 0x9.ea2025b4c56553f5cdee4c924728p-92L},
	{0x1.d5818dcfba48725da05aeap0L, 0x1.66e0dca9f589f559c0876ff23830p-88L},
	{0x1.d80e316c98397bb84f9d04p0L, 0x8.805f84bec614de269900ddf98d28p-92L},
	{0x1.da9e603db3285708c01a5ap0L, 0x1.6d4c97f6246f0ec614ec95c99392p-88L},
	{0x1.dd321f301b4604b695de3cp0L, 0x6.30a393215299e30d4fb73503c348p-96L},
	{0x1.dfc97337b9b5eb968cac38p0L, 0x1.ed291b7225a944efd5bb5524b927p-88L},
	{0x1.e264614f5a128a12761fa0p0L, 0x1.7ada6467e77f73bf65e04c95e29dp-88L},
	{0x1.e502ee78b3ff6273d13014p0L, 0x1.3991e8f49659e1693be17ae1d2f9p-88L},
	{0x1.e7a51fbc74c834b548b282p0L, 0x1.23786758a84f4956354634a416cep-88L},
	{0x1.ea4afa2a490d9858f73a18p0L, 0xf.5db301f86dea20610ceee13eb7b8p-92L},
	{0x1.ecf482d8e67f08db0312fap0L, 0x1.949cef462010bb4bc4ce72a900dfp-88L},
	{0x1.efa1bee615a27771fd21a8p0L, 0x1.2dac1f6dd5d229ff68e46f27e3dfp-88L},
	{0x1.f252b376bba974e8696fc2p0L, 0x1.6390d4c6ad5476b5162f40e1d9a9p-88L},
	{0x1.f50765b6e4540674f84b76p0L, 0x2.862baff99000dfc4352ba29b8908p-92L},
	{0x1.f7bfdad9cbe138913b4bfep0L, 0x7.2bd95c5ce7280fa4d2344a3f5618p-92L},
	{0x1.fa7c1819e90d82e90a7e74p0L, 0xb.263c1dc060c36f7650b4c0f233a8p-92L},
	{0x1.fd3c22b8f71f10975ba4b2p0L, 0x1.2bcf3a5e12d269d8ad7c1a4a8875p-88L}
};

/*
 * Kernel for expl(x).  x must be finite and not tiny or huge.
 * "tiny" is anything that would make us underflow (|A6*x^6| < ~LDBL_MIN).
 * "huge" is anything that would make fn*L1 inexact (|x| > ~2**17*ln2).
 */
static inline void
__k_expl(long double x, long double *hip, long double *lop, int *kp)
{
	long double q, r, r1, t;
	double dr, fn, r2;
	int n, n2;

	/* Reduce x to (k*ln2 + endpoint[n2] + r1 + r2). */
	fn = rnint((double)x * INV_L);
	n = irint(fn);
	n2 = (unsigned)n % INTERVALS;
	/* Depend on the sign bit being propagated: */
	*kp = n >> LOG2_INTERVALS;
	r1 = x - fn * L1;
	r2 = fn * -L2;
	r = r1 + r2;

	/* Evaluate expl(endpoint[n2] + r1 + r2) = tbl[n2] * expl(r1 + r2). */
	dr = r;
	q = r2 + r * r * (A2 + r * (A3 + r * (A4 + r * (A5 + r * (A6 +
	    dr * (A7 + dr * (A8 + dr * (A9 + dr * A10))))))));
	t = tbl[n2].lo + tbl[n2].hi;
	*hip = tbl[n2].hi;
	*lop = tbl[n2].lo + t * (q + r1);
}

/*
 * XXX: the rest of the functions are identical for ld80 and ld128.
 * However, we should use scalbnl() for ld128, since long double
 * multiplication was very slow on sparc64 and no new evaluation has
 * been made for aarch64 and/or riscv.
 */

static inline void
k_hexpl(long double x, long double *hip, long double *lop)
{
	float twopkm1;
	int k;

	__k_expl(x, hip, lop, &k);
	SET_FLOAT_WORD(twopkm1, 0x3f800000 + ((k - 1) << 23));
	*hip *= twopkm1;
	*lop *= twopkm1;
}

static inline long double
hexpl(long double x)
{
	long double hi, lo, twopkm2;
	int k;

	twopkm2 = 1;
	__k_expl(x, &hi, &lo, &k);
	SET_LDBL_EXPSIGN(twopkm2, BIAS + k - 2);
	return (lo + hi) * 2 * twopkm2;
}

#ifdef _COMPLEX_H
/*
 * See ../src/k_exp.c for details.
 */
static inline long double complex
__ldexp_cexpl(long double complex z, int expt)
{
	long double c, exp_x, hi, lo, s;
	long double x, y, scale1, scale2;
	int half_expt, k;

	x = creall(z);
	y = cimagl(z);
	__k_expl(x, &hi, &lo, &k);

	exp_x = (lo + hi) * 0x1p16382L;
	expt += k - 16382;

	scale1 = 1;
	half_expt = expt / 2;
	SET_LDBL_EXPSIGN(scale1, BIAS + half_expt);
	scale2 = 1;
	SET_LDBL_EXPSIGN(scale2, BIAS + expt - half_expt);

	sincosl(y, &s, &c);
	return (CMPLXL(c * exp_x * scale1 * scale2,
	    s * exp_x * scale1 * scale2));
}
#endif /* _COMPLEX_H */

COSMOPOLITAN_C_END_
#endif /* !(__ASSEMBLER__ + __LINKER__ + 0) */
#endif /* COSMOPOLITAN_LIBC_TINYMATH_FREEBSD_INTERNAL_H_ */
