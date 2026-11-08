/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include "../opm.c"

/* Original serial multiplier: preserve every clock's output and carry. */
static void reference_lfo_mult(opm_t *chip)
{
    uint8_t ampm_sel = (chip->lfo_bit_counter & 8) != 0;
    uint8_t dp = ampm_sel ? chip->lfo_pmd : chip->lfo_amd;
    uint8_t bit = 0, b1, b2;
    uint8_t sum;

    chip->lfo_out2_b = chip->lfo_out2;

    switch (chip->lfo_bit_counter & 7)
    {
    case 0:
        bit = (dp & 64) != 0 && (chip->lfo_out1 & 64) != 0;
        break;
    case 1:
        bit = (dp & 32) != 0 && (chip->lfo_out1 & 32) != 0;
        break;
    case 2:
        bit = (dp & 16) != 0 && (chip->lfo_out1 & 16) != 0;
        break;
    case 3:
        bit = (dp & 8) != 0 && (chip->lfo_out1 & 8) != 0;
        break;
    case 4:
        bit = (dp & 4) != 0 && (chip->lfo_out1 & 4) != 0;
        break;
    case 5:
        bit = (dp & 2) != 0 && (chip->lfo_out1 & 2) != 0;
        break;
    case 6:
        bit = (dp & 1) != 0 && (chip->lfo_out1 & 1) != 0;
        break;
    }

    b1 = (chip->lfo_out2 & 1) != 0;
    if ((chip->lfo_bit_counter & 7) == 0)
    {
        b1 = 0;
    }
    b2 = chip->lfo_mult_carry;
    if ((chip->cycles & 15) == 15)
    {
        b2 = 0;
    }
    sum = bit + b1 + b2;
    chip->lfo_out2 >>= 1;
    chip->lfo_out2 |= (sum & 1) << 15;
    chip->lfo_mult_carry = sum >> 1;
}


int main(void)
{
    opm_t actual = {0}, expected;
    for (unsigned counter = 0; counter < 16; counter++)
    for (unsigned depth = 0; depth < 128; depth++)
    for (unsigned wave = 0; wave < 256; wave++)
    for (unsigned state = 0; state < 8; state++)
    {
        actual.lfo_bit_counter = counter;
        actual.lfo_pmd = depth;
        actual.lfo_amd = 127 - depth;
        actual.lfo_out1 = wave;
        actual.lfo_out2 = 0xa55a | (state & 1);
        actual.lfo_mult_carry = (state >> 1) & 1;
        actual.cycles = (state & 4) ? 15 : 0;
        expected = actual;
        reference_lfo_mult(&expected);
        OPM_DoLFOMult(&actual);
        assert(actual.lfo_out2_b == expected.lfo_out2_b);
        assert(actual.lfo_out2 == expected.lfo_out2);
        assert(actual.lfo_mult_carry == expected.lfo_mult_carry);
    }
    return 0;
}
