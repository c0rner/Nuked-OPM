/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include "../opm.c"

/* Compare cached phase generation against the original pure arithmetic while
 * varying every dependency, revisiting slots, and exercising YM2164 delays. */
static void check_pitch_cache(void)
{
    uint32_t random = 1;
    for (uint32_t opp = 0; opp < 2; opp++)
    {
        opm_t chip;
        OPM_Reset(&chip, opp ? opm_flags_ym2164 : opm_flags_none);
        for (uint32_t step = 0; step < 20000; step++)
        {
            uint32_t slot = (chip.cycles + 7) & 31;
            uint32_t channel = slot & 7;
            random = random * 1664525u + 1013904223u;
            /* Include repeated inputs as well as changes to each key field. */
            switch ((step / 32) % 8)
            {
            case 0: chip.ch_kc[channel] = random & 127; break;
            case 1: chip.ch_kf[channel] = random & 63; break;
            case 2: chip.lfo_pmd = random & 127; break;
            case 3: chip.lfo_pm_lock = random & 255; break;
            case 4: chip.ch_pms[channel] = random & 7;
                    chip.pg_opp_pms = (random >> 8) & 7; break;
            case 5: chip.sl_dt2[slot] = random & 3;
                    chip.pg_opp_dt2[slot] = (random >> 8) & 3; break;
            default: break;
            }
            uint32_t kcf = (chip.ch_kc[channel] << 6) + chip.ch_kf[channel];
            uint32_t lfo = chip.lfo_pmd ? chip.lfo_pm_lock : 0;
            uint32_t pms = opp ? chip.pg_opp_pms : chip.ch_pms[channel];
            uint32_t dt = opp ? chip.pg_opp_dt2[slot] : chip.sl_dt2[slot];
            uint32_t code = OPM_CalcKCode(kcf, OPM_LFOApplyPMS(lfo & 127, pms),
                (lfo & 128) != 0 && pms != 0 ? 0 : 1, dt);
            OPM_PhaseCalcFNumBlock(&chip);
            assert(chip.pg_fnum[slot] == OPM_KCToFNum(code));
            assert(chip.pg_kcode[slot] == code >> 8);
            if (opp)
            {
                uint32_t pipeline_slot = chip.cycles & 31;
                assert(chip.pg_opp_pms == chip.ch_pms[pipeline_slot & 7]);
                assert(chip.pg_opp_dt2[pipeline_slot] == chip.sl_dt2[pipeline_slot]);
            }
            chip.cycles = (chip.cycles + 1) & 31;
        }
        OPM_Reset(&chip, opp ? opm_flags_ym2164 : opm_flags_none);
        for (uint32_t slot = 0; slot < 32; slot++)
            assert(chip.pg_pitch_key[slot] == 0 ||
                   chip.pg_pitch_key[slot] == 0x80000000u);
        /* Reset clocks the core while IC is active, so zero-input cache
         * entries may already be valid when it returns. */
    }
}

int main(void)
{
    check_pitch_cache();
    return 0;
}
