#if 0
	//{AFE4404_CONTROL0	,0x08},	//0
	//{AFE4404_DELAY		,0x01},

	{AFE4404_LED2STC	,0x50},
	{AFE4404_LED2ENDC	,0x619},
	{AFE4404_LED1LEDSTC	,0xC34},
	{AFE4404_LED1LEDENDC	,0x124D},

	{AFE4404_ALED2STC	,0x66A},	//6
	{AFE4404_ALED2ENDC	,0xC33},
	{AFE4404_LED1STC	,0xC84},
	{AFE4404_LED1ENDC	,0x124D},
	{AFE4404_LED2LEDSTC	,0x0},
	{AFE4404_LED2LEDENDC	,0x619},
	{AFE4404_ALED1STC	,0x129F},
	{AFE4404_ALED1ENDC	,0x1868},
	{AFE4404_LED2CONVST	,0x622},
	{AFE4404_LED2CONVEND	,0xC3A},
	{AFE4404_ALED2CONVST	,0xC43},
	{AFE4404_ALED2CONVEND	,0x125B},
	{AFE4404_LED1CONVST		,0x1264},
	{AFE4404_LED1CONVEND	,0x187C},
	{AFE4404_ALED1CONVST	,0x1885},
	{AFE4404_ALED1CONVEND	,0x1E9D},
	{AFE4404_ADCRSTSTCT0	,0x61B},
	{AFE4404_ADCRSTENDCT0	,0x621},
	{AFE4404_ADCRSTSTCT1	,0xC3C},
	{AFE4404_ADCRSTENDCT1	,0xC42},
	{AFE4404_ADCRSTSTCT2	,0x125D},
	{AFE4404_ADCRSTENDCT2	,0x1263},
	{AFE4404_ADCRSTSTCT3	,0x187E},
	{AFE4404_ADCRSTENDCT3	,0x1884},
	{AFE4404_PRPCOUNT	,0x7A11},
	{AFE4404_CONTROL1	,0x103},
	{AFE4404_TIAGAIN	,0x8003},
	{AFE4404_TIA_AMB_GAIN	,0x3},
	{AFE4404_LEDCNTRL	,0xC},
	{AFE4404_CONTROL2	,0x104218},
	{AFE4404_CLKDIV1	,0x0},
	//{AFE4404_LED2VAL	,0xFFBE99},
	//{AFE4404_ALED2VAL	,0xFFD6E5},
	//{AFE4404_LED1VAL	,0xFFBF3C},
	//{AFE4404_ALED1VAL	,0x192C6F},
	//{AFE4404_LED2-ALED2VAL	,0xFFF906},
	//{AFE4404_LED1-ALED1VAL	,0xE662AD},
	{AFE4404_DIAG	,0x0},
	{AFE4404_CONTROL3	,0x0},
	{AFE4404_PDNCYCLESTC	,0x21BD},
	{AFE4404_PDNCYCLEENDC	,0x76F0},
	{AFE4404_PROG_TG_STC	,0x0},
	{AFE4404_PROG_TG_ENDC	,0x0},
	{AFE4404_LED3LEDSTC	,0x61A},
	{AFE4404_LED3LEDENDC	,0xC33},
	{AFE4404_CLKDIV2	,0x0},
	{AFE4404_OFFDAC		,0x0},
#else
#if 0
//{AFE4404_CONTROL0	, 0x0000},
{AFE4404_LED2STC	, 0x0050},
{AFE4404_LED2ENDC	, 0x0619},
{AFE4404_LED1LEDSTC	, 0x0C34},
{AFE4404_LED1LEDENDC	, 0x124D},
{AFE4404_ALED2STC	, 0x066A},
{AFE4404_ALED2ENDC	, 0x0C33},
{AFE4404_LED1STC	, 0x0C84},
{AFE4404_LED1ENDC	, 0x124D},
{AFE4404_LED2LEDSTC	, 0x0000},
{AFE4404_LED2LEDENDC	, 0x0619},
{AFE4404_ALED1STC	, 0x129F},
{AFE4404_ALED1ENDC	, 0x1868},
{AFE4404_LED2CONVST	, 0x0622},
{AFE4404_LED2CONVEND	, 0x0C3A},
{AFE4404_ALED2CONVST	, 0x0C43},
{AFE4404_ALED2CONVEND	, 0x125B},
{AFE4404_LED1CONVST	, 0x0000},
{AFE4404_LED1CONVEND	, 0x187C},
{AFE4404_ALED1CONVST	, 0x1885},
{AFE4404_ALED1CONVEND	, 0x1E9D},
{AFE4404_ADCRSTSTCT0	, 0x061B},
{AFE4404_ADCRSTENDCT0	, 0x0621},
{AFE4404_ADCRSTSTCT1	, 0x0C3C},
{AFE4404_ADCRSTENDCT1	, 0x0C42},
{AFE4404_ADCRSTSTCT2	, 0x125D},
{AFE4404_ADCRSTENDCT2	, 0x1263},
{AFE4404_ADCRSTSTCT3	, 0x187E},
{AFE4404_ADCRSTENDCT3	, 0x1884},
{AFE4404_PRPCOUNT	, 0x7A11},
{AFE4404_CONTROL1	, 0x0103},
{AFE4404_TIAGAIN	, 0x8003},
{AFE4404_TIA_AMB_GAIN	, 0x0003},
{AFE4404_LEDCNTRL	, 0xC30C},
{AFE4404_CONTROL2	, 0x104218},
{AFE4404_CLKDIV1	, 0x0000},
//{AFE4404_LED2VAL	, 0x0000},
//{AFE4404_ALED2VAL	, 0x0000},
//{AFE4404_LED1VAL	, 0x0000},
//{AFE4404_ALED1VAL	, 0x0000},
//{AFE4404_LED2-ALED2VAL	, 0x0000},
//{AFE4404_LED1-ALED1VAL	, 0x0000},
{AFE4404_DIAG	, 0x0000},
{AFE4404_CONTROL3	, 0x0000},
{AFE4404_PDNCYCLESTC	, 0x21BD},
{AFE4404_PDNCYCLEENDC	, 0x76F0},
{AFE4404_PROG_TG_STC	, 0x0000},
{AFE4404_PROG_TG_ENDC	, 0x0000},
{AFE4404_LED3LEDSTC	, 0x061A},
{AFE4404_LED3LEDENDC	, 0x0C33},
{AFE4404_CLKDIV2	, 0x0000},
{AFE4404_OFFDAC	, 0x0000},
#endif
{0x01,0x00004E},
{0x02,0x000185},
{0x03,0x000B66},
{0x04,0x000CEB},
{0x05,0x000601},
{0x06,0x000738},
{0x07,0x000BB4},
{0x08,0x000CEB},
{0x09,0x000000},
{0x0A,0x000185},
{0x0B,0x001167},
{0x0C,0x00129E},
{0x0D,0x00018E},
{0x0E,0x0005B1},
{0x0F,0x000741},
{0x10,0x000B64},
{0x11,0x000CF4},
{0x12,0x001117},
{0x13,0x0012A7},
{0x14,0x0016CA},
{0x15,0x000187},
{0x16,0x00018D},
{0x17,0x00073A},
{0x18,0x000740},
{0x19,0x000CED},
{0x1A,0x000CF3},
{0x1B,0x0012A0},
{0x1C,0x0012A6},
{0x1D,0x007A11},
{0x1E,0x000103},
{0x20,0x008002},
{0x21,0x000002},
{0x22,0x00C30D},
{0x23,0x104218},
{0x29,0x000000},
{0x32,0x0019EA},
{0x33,0x0076F1},
{0x36,0x0005B3},
{0x37,0x000738},
{0x39,0x000000},
{0x3A,0x000000},
#endif
