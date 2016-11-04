#include <stdio.h>

int bin2decimal(const uchar *from, decimal_t *to, int precision, int scale)
{
  int error = E_DEC_OK,
  int intg = precision - scale,
  int intg0 = intg / DIG_PER_DEC1,
  int frac0 = scale / DIG_PER_DEC1,
  int intg0x = intg - intg0 * DIG_PER_DEC1,
  int frac0x = scale - frac0 * DIG_PER_DEC1,
  int intg1 = intg0 + (intg0x > 0),
  int frac1 = frac0 + (frac0x > 0);
  dec1 *buf = to->buf, mask = (*from & 0x80) ? 0 : -1;
  const uchar *stop;
  uchar *d_copy;
  int bin_size = decimal_bin_size(precision, scale);

  sanity(to);
  d_copy= (uchar*) my_alloca(bin_size);
  memcpy(d_copy, from, bin_size);
  d_copy[0] ^= 0x80;
  from= d_copy;

  FIX_INTG_FRAC_ERROR(to->len, intg1, frac1, error);
  if (unlikely(error))
  {
    if (intg1 < intg0+(intg0x>0))
    {
      from+=dig2bytes[intg0x]+sizeof(dec1)*(intg0-intg1);
      frac0=frac0x=intg0x=0;
      intg0=intg1;
    }
    else
    {
      frac0x=0;
      frac0=frac1;
    }
  }

  to->sign=(mask != 0);
  to->intg= intg0 * DIG_PER_DEC1 + intg0x;
  to->frac= frac0 * DIG_PER_DEC1 + frac0x;

  if (intg0x)
  {
    int i=dig2bytes[intg0x];
    dec1 UNINIT_VAR(x);
    switch (i)
    {
      case 1: x=mi_sint1korr(from); break;
      case 2: x=mi_sint2korr(from); break;
      case 3: x=mi_sint3korr(from); break;
      case 4: x=mi_sint4korr(from); break;
      default: DBUG_ASSERT(0);
    }
    from+=i;
    *buf=x ^ mask;
    if (((ulonglong)*buf) >= (ulonglong) powers10[intg0x+1])
      goto err;
    if (buf > to->buf || *buf != 0)
      buf++;
    else
      to->intg-=intg0x;
  }
  for (stop=from+intg0*sizeof(dec1); from < stop; from+=sizeof(dec1))
  {
    DBUG_ASSERT(sizeof(dec1) == 4);
    *buf=mi_sint4korr(from) ^ mask;
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    if (buf > to->buf || *buf != 0)
      buf++;
    else
      to->intg-=DIG_PER_DEC1;
  }
  DBUG_ASSERT(to->intg >=0);
  for (stop=from+frac0*sizeof(dec1); from < stop; from+=sizeof(dec1))
  {
    DBUG_ASSERT(sizeof(dec1) == 4);
    *buf=mi_sint4korr(from) ^ mask;
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    buf++;
  }
  if (frac0x)
  {
    int i=dig2bytes[frac0x];
    dec1 UNINIT_VAR(x);
    switch (i)
    {
      case 1: x=mi_sint1korr(from); break;
      case 2: x=mi_sint2korr(from); break;
      case 3: x=mi_sint3korr(from); break;
      case 4: x=mi_sint4korr(from); break;
      default: DBUG_ASSERT(0);
    }
    *buf=(x ^ mask) * powers10[DIG_PER_DEC1 - frac0x];
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    buf++;
  }
  my_afree(d_copy);

  /*
    No digits? We have read the number zero, of unspecified precision.
    Make it a proper zero, with non-zero precision.
  */
  if (to->intg == 0 && to->frac == 0)
    decimal_make_zero(to);
  return error;

err:
  my_afree(d_copy);
  decimal_make_zero(to);
  return(E_DEC_BAD_NUM);
}