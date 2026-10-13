#pragma once
// All five flags: 0 = frozen Mobile ON; 1 = matched control.
#ifndef AQCF_ABLATE_PREFIX
#define AQCF_ABLATE_PREFIX 0
#endif
static_assert(AQCF_ABLATE_PREFIX==0||AQCF_ABLATE_PREFIX==1,"boolean Mobile ablation");
#ifndef AQCF_ABLATE_EMITTER
#define AQCF_ABLATE_EMITTER 0
#endif
static_assert(AQCF_ABLATE_EMITTER==0||AQCF_ABLATE_EMITTER==1,"boolean Mobile ablation");
#ifndef AQCF_ABLATE_QUOTIENT
#define AQCF_ABLATE_QUOTIENT 0
#endif
static_assert(AQCF_ABLATE_QUOTIENT==0||AQCF_ABLATE_QUOTIENT==1,"boolean Mobile ablation");
#ifndef AQCF_ABLATE_SHORT
#define AQCF_ABLATE_SHORT 0
#endif
static_assert(AQCF_ABLATE_SHORT==0||AQCF_ABLATE_SHORT==1,"boolean Mobile ablation");
#ifndef AQCF_ABLATE_FUSION
#define AQCF_ABLATE_FUSION 0
#endif
static_assert(AQCF_ABLATE_FUSION==0||AQCF_ABLATE_FUSION==1,"boolean Mobile ablation");
