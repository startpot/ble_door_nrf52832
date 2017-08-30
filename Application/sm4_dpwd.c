// SM4
#include <stdbool.h>
#include <stdint.h>
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "sm4_mcu.h"
#include "sm4_dpwd.h"

#ifdef __cpluseplus
#define INLINE INLINE
#else
#define INLINE
#endif

#define  sm_word			uint32_t

/***********************************
*32位大小端变换
************************************/
INLINE uint32_t Reverse32(uint32_t x)
{
	uint32_t tmp;
	tmp = (x & 0x000000ff) << 24;
	tmp |= (x & 0x0000ff00) << 8;
	tmp |= (x & 0x00ff0000) >> 8;
	tmp |= (x & 0xff000000) >> 24;
	return tmp;
}

/*******************************************
*64位大小端变换
********************************************/
INLINE uint64_t Reverse64(uint64_t x)
{
	uint32_t nTemp[3] = {0};
	memcpy(nTemp + 1, &x, sizeof(uint64_t));
	nTemp[0] = Reverse32(nTemp[2]);
	nTemp[1] = Reverse32(nTemp[1]);
	return *(uint64_t *)nTemp;
}

INLINE sm_word ML(uint8_t X, uint8_t j)
{
	return Reverse32((sm_word)(X << (j % 32)));
}

INLINE sm_word SUM(sm_word X, sm_word Y)
{
	return Reverse32(Reverse32(X) + Reverse32(Y));
}

int TruncateSM4(uint8_t pSrc[16], uint16_t nSrcLen, uint8_t pDst[4], uint16_t nDstSize)
{
	if(nSrcLen != 16 || nDstSize < 4)
	{
		return -1;
	}

	memset(pDst, 0, nDstSize);

	uint8_t* S = (uint8_t *)pSrc;
	sm_word S1 = ML(S[ 0], 24) | ML(S[ 1], 16) | ML(S[ 2], 8) | ML(S[ 3], 0);
	sm_word S2 = ML(S[ 4], 24) | ML(S[ 5], 16) | ML(S[ 6], 8) | ML(S[ 7], 0);
	sm_word S3 = ML(S[ 8], 24) | ML(S[ 9], 16) | ML(S[10], 8) | ML(S[11], 0);
	sm_word S4 = ML(S[12], 24) | ML(S[13], 16) | ML(S[14], 8) | ML(S[15], 0);

	sm_word OD = SUM(SUM(SUM(S1, S2), S3), S4);
	memcpy(pDst, &OD, sizeof(sm_word));

	return 0;
}

// pKey：种子数组，128位，16uint8_t；不足128bits，自动填充‘0’补足长度
// pTime：时间因子，UTC或自定义时间，8uint8_t，单位秒
// pInterval：时间间隔，最大为60秒
// pCounter：事件因子，4uint8_t
// pChallenge：挑战因子或其他需要参与运算的因子，最小4uint8_t
// pDynPwd：输出的动态口令，可配置位数nDynPwdSize
// nGenLen: 输出的10进制的位数
// nDynPwdSize: 用于存储十进制动态密码的uint8_t个数
// 
#define DPWD_KEY_LEN					16
#define DPWD_UTC_TIME_LEN				8
#define DPWD_TIME_INTERVAL_LEN			2
#define DPWD_COUNTER_LEN				4
#define DPWD_CHALLENGE_LEN				4
#define DPWD_GEN_DPWD_DEC_LEN			6
#define DPWD_GEN_DPWD_HEX_LEN			6

#define DPWD_ERROR_OK					0
#define DPWD_ERROR_INTERVAL_ZERO		1
#define DPWD_ERROR_INTERVAL_TOO_LARGE	2
#define DPWD_ERROR_OTHER				3
// Key：种子，16字节
// Time：UTC或其他格式的时间，8uint8_t，单位秒，Littel-endian
// Interval：时间间隔，2uint8_t，单位秒，Little-endian
// Counter：计数值，4uint8_t，Littel-endian
// Challenge：挑战数，4uint8_t, 自定义
// Dynpwd：输出的动态口令，ASIIC，十进制数值
// 种子和时间是必须的；Interval最大为60秒；Counter和Challenge填写固定值，或者用户定义
// DPWD_UTC_TIME_LEN + DPWD_COUNTER_LEN + DPWD_CHALLENGE_LEN == 16
// 若DPWD_KEY_LEN 和 （DPWD_UTC_TIME_LEN + DPWD_COUNTER_LEN + DPWD_CHALLENGE_LEN）不等于16，应补充0到16uint8_t
// 如果种子和ID的长度大于16uint8_t，应按标准做多次处理。以下仅仅针对16uint8_t
int SM4_DPasswd(uint8_t * pKey, uint64_t Time, uint16_t Interval, uint32_t Counter, \
					uint8_t* pChallenge, uint8_t* pDynPwd)
{
	// T = T0/Tc
	if(Interval == 0)
		return DPWD_ERROR_INTERVAL_ZERO;
	if(Interval > 60)
		return DPWD_ERROR_INTERVAL_TOO_LARGE;
	
	uint64_t tTime;
	tTime = Time / Interval;

	uint8_t sm_k[DPWD_KEY_LEN] = {0};
	uint8_t sm_i[DPWD_UTC_TIME_LEN + DPWD_COUNTER_LEN + DPWD_CHALLENGE_LEN] = {0};
	uint8_t sm_o[DPWD_KEY_LEN] = {0};

	int i, offset;
	uint32_t pwd = 0;
	
	// copy key
	for(i = 0; i < DPWD_KEY_LEN; i ++)
	{
		sm_k[i] = pKey[i];
	}
	
	// assemble ID
	// ID = T|C|Q
	// ID 由T C Q构成；T C最少必须有一个存在；Q为可选项；若ID不足128bits，则自动补充‘0’填充满
	// 将T、C、Q copy到sm_buf中；不足部分自动补充'0'
	uint8_t* tBuf;
	tTime = Reverse64(tTime);
	tBuf = (uint8_t *)&tTime;
	for(i = 0, offset = 0; i < DPWD_UTC_TIME_LEN; i ++)
	{
		sm_i[offset ++] = tBuf[i];
	}
	
	uint32_t tCounter;
	tCounter = Reverse32(Counter);
	tBuf = (uint8_t *)&tCounter;
	for(i = 0; i < DPWD_COUNTER_LEN; i ++)
	{
		sm_i[offset ++] = tBuf[i];
	}
	for(i = 0; i < DPWD_CHALLENGE_LEN; i ++)
	{
		sm_i[offset ++] = pChallenge[i];
	}

	struct sm4_context ctx;
	sm4_setkey_enc(&ctx, sm_k);
	sm4_crypt_ecb(&ctx, SM4_ENCRYPT, 16, sm_i, sm_o);

	TruncateSM4(sm_o, DPWD_KEY_LEN, (uint8_t*) &pwd, sizeof(pwd));

	pwd = Reverse32(pwd);

	pwd = pwd % (int)pow(10, DPWD_GEN_DPWD_DEC_LEN);

	for(i = DPWD_GEN_DPWD_DEC_LEN - 1; i >= 0; i --)
	{
		uint32_t tmp;
		tmp = pow(10, i);
		offset = pwd / tmp;
		pwd %= tmp;
		pDynPwd [i] = offset + 0x30;
	}
	
	return DPWD_ERROR_OK;

}
