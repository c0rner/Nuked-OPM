/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include "../opm.c"

/* Uncached arithmetic and clock ordering serve as the reference. */
static void reference_increment(opm_t *chip)
{
    uint32_t slot = chip->cycles;
    uint32_t channel = slot & 7;
    uint32_t dt = chip->sl_dt1[slot];
    uint32_t dt_l = dt & 3;
    uint32_t detune = 0;
    uint32_t multi = chip->sl_mul[slot];
    uint32_t kcode = chip->pg_kcode[slot];
    uint32_t fnum = chip->pg_fnum[slot];
    uint32_t block = kcode >> 2;
    uint32_t basefreq = (fnum << block) >> 2;
    uint32_t note, sum, sum_h, sum_l, inc;
    /* Apply detune */
    if (dt_l)
    {
        if (kcode > 0x1c)
        {
            kcode = 0x1c;
        }
        block = kcode >> 2;
        note = kcode & 0x03;
        sum = block + 9 + ((dt_l == 3) | (dt_l & 0x02));
        sum_h = sum >> 1;
        sum_l = sum & 0x01;
        detune = pg_detune[(sum_l << 2) | note] >> (9 - sum_h);
    }
    if (dt & 0x04)
    {
        basefreq -= detune;
    }
    else
    {
        basefreq += detune;
    }
    basefreq &= 0x1ffff;
    if (multi)
    {
        inc = basefreq * multi;
    }
    else
    {
        inc = basefreq >> 1;
    }
    inc &= 0xfffff;
    chip->pg_inc[slot] = inc;
}

void reference_clock(opm_t *chip, int32_t *output, uint8_t *sh1, uint8_t *sh2, uint8_t *so)
{
    OPM_Output(chip);
    OPM_DAC(chip);
    OPM_Mixer2(chip);
    OPM_Mixer(chip);

    OPM_OperatorPhase16(chip);
    OPM_OperatorPhase15(chip);
    OPM_OperatorPhase14(chip);
    OPM_OperatorPhase13(chip);
    OPM_OperatorPhase12(chip);
    OPM_OperatorPhase11(chip);
    OPM_OperatorPhase10(chip);
    OPM_OperatorPhase9(chip);
    OPM_OperatorPhase8(chip);
    OPM_OperatorPhase7(chip);
    OPM_OperatorPhase6(chip);
    OPM_OperatorPhase5(chip);
    OPM_OperatorPhase4(chip);
    OPM_OperatorPhase3(chip);
    OPM_OperatorPhase2(chip);
    OPM_OperatorPhase1(chip);
    OPM_OperatorCounter(chip);

    OPM_EnvelopeTimer(chip);
    OPM_EnvelopePhase6(chip);
    OPM_EnvelopePhase5(chip);
    OPM_EnvelopePhase4(chip);
    OPM_EnvelopePhase3(chip);
    OPM_EnvelopePhase2(chip);
    OPM_EnvelopePhase1(chip);

    if (chip->opp)
        OPP_TLRamp(chip);

    OPM_PhaseDebug(chip);
    OPM_PhaseGenerate(chip);
    reference_increment(chip);
    OPM_PhaseCalcFNumBlock(chip);

    OPM_DoTimerIRQ(chip);
    OPM_DoTimerA(chip);
    OPM_DoTimerB(chip);
    OPM_DoLFOMult(chip);
    OPM_DoLFO1(chip);
    OPM_Noise(chip);
    OPM_KeyOn2(chip);
    OPM_DoRegWrite(chip);
    OPM_EnvelopeClock(chip);
    OPM_NoiseTimer(chip);
    OPM_KeyOn1(chip);
    OPM_DoIO(chip);
    OPM_DoTimerA2(chip);
    OPM_DoTimerB2(chip);
    OPM_DoLFO2(chip);
    OPM_CSM(chip);
    OPM_NoiseChannel(chip);
    OPM_DoIC(chip);
    if (sh1)
    {
        *sh1 = chip->smp_sh1;
    }
    if (sh2)
    {
        *sh2 = chip->smp_sh2;
    }
    if (so)
    {
        *so = chip->smp_so;
    }
    if (output)
    {
        output[0] = chip->dac_output[0];
        output[1] = chip->dac_output[1];
    }
    chip->cycles = (chip->cycles + 1) & 31;
}


int main(void)
{
    opm_t actual, expected;
    for (unsigned opp = 0; opp < 2; opp++)
    {
        OPM_Reset(&actual, opp ? opm_flags_ym2164 : opm_flags_none);
        expected = actual;
        for (unsigned code = 0; code < 32; code++)
        for (unsigned dt = 0; dt < 8; dt++)
        for (unsigned mul = 0; mul < 16; mul++)
        for (unsigned frac = 0; frac < 1024; frac++)
        {
            unsigned slot = frac & 31;
            actual.cycles = expected.cycles = slot;
            actual.pg_fnum[slot] = expected.pg_fnum[slot] = OPM_KCToFNum(frac);
            actual.pg_kcode[slot] = expected.pg_kcode[slot] = code;
            actual.sl_dt1[slot] = expected.sl_dt1[slot] = dt;
            actual.sl_mul[slot] = expected.sl_mul[slot] = mul;
            reference_increment(&expected);
            OPM_PhaseCalcIncrement(&actual);
            assert(actual.pg_inc[slot] == expected.pg_inc[slot]);
            /* A cache hit must restore an increment cleared by phase reset. */
            actual.pg_inc[slot] = 0;
            OPM_PhaseCalcIncrement(&actual);
            assert(actual.pg_inc[slot] == expected.pg_inc[slot]);
        }
        OPM_Reset(&actual, opp ? opm_flags_ym2164 : opm_flags_none);
        expected = actual;
        unsigned random = 1;
        for (unsigned step = 0; step < 200000; step++)
        {
            random = random * 1664525u + 1013904223u;
            if ((step & 63) == 0 || (step & 63) == 8)
            {
                unsigned port = (step & 63) == 8;
                OPM_Write(&actual, port, random >> 24);
                OPM_Write(&expected, port, random >> 24);
            }
            if (step % 10000 == 0 || step % 10000 == 64)
            {
                OPM_SetIC(&actual, step % 10000 == 0);
                OPM_SetIC(&expected, step % 10000 == 0);
            }
            OPM_Clock(&actual, 0, 0, 0, 0);
            reference_clock(&expected, 0, 0, 0, 0);
            /* Compare every pipeline field, excluding optimization metadata. */
            memcpy(expected.pg_inc_key, actual.pg_inc_key, sizeof(actual.pg_inc_key));
            memcpy(expected.pg_inc_cached, actual.pg_inc_cached, sizeof(actual.pg_inc_cached));
            assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
        }
    }
    return 0;
}
