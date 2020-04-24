#include <include/types.h>

#ifdef CONFIG_PLATFORM_NRFS
#include <nrf_delay.h>
#else
#error "please check delay funtion on this platform!!!"
#endif

void __WEAK__ udelay(__u32 n)
{
#ifdef CONFIG_PLATFORM_NRFS
	nrf_delay_us(n);
#else
	volatile __u32 m = n * (HCLK_RATE >> 20) >> 6;

	while (m-- > 0);
#endif
}

void __WEAK__ mdelay(__u32 n)
{
	udelay(1000 * n);
}