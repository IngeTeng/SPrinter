#ifndef WARNING_TYPE_H
#define WARNING_TYPE_H


typedef struct Waring
{
	unsigned short bit_idx;
	const char *wcode;
	const char *warning_str;
}Warning;

#define  W1   12
#define  W2   9



// 0x27 reg  - status[0]
//Warning  warning_1[W1]=
//{
//  {0x0080,"C3451","副预热温度异常"},
//	{0x0040,"C2702","主预热温度异常"},
//	{0x0020,"C5315","定影冷却风扇马达故障"},
//	{0x0010,"C2351","冷却风扇马达故障"},
//	{0x0008,"C5102","主马达故障"},
//	{0x0004,"C2351","多棱镜马达旋转故障"},
//	{0x0002,"C03FF","定影温度异常 副"},
//	{0x0001,"C0211","定影温度异常 主"}
//};
Warning  warning_1[W1]=
{
	{0x0800,"C255c","C255c"},
	{0x0400,"C2558","C2558"},
	{0x0200,"C2557","C2557"},
	{0x0100,"C0000","碳粉耗尽"},
  	{0x0080,"C3451","副预热温度异常"},
	{0x0040,"C2702","主预热温度异常"},
	{0x0020,"C5315","定影冷却风扇马达故障"},
	{0x0010,"C2351","冷却风扇马达故障"},
	{0x0008,"C5102","主马达故障"},
	{0x0004,"C2351","多棱镜马达旋转故障"},
	{0x0002,"C03FF","定影温度异常 副"},
	{0x0001,"C0211","定影温度异常 主"}
};

// 0x2A Reg  - status[1]
Warning  warning_2[W2]=
{
	{0x0100, "InkInjecting", "碳粉即将用尽"},//碳粉用尽仅提示，不做暂停动作
    {0x0080, "NoPaper", "纸盒缺纸"},
    {0x0040, "HandNoPaper", "手动纸盘缺纸"}, //该位高表示手动无纸。　
	{0x0020, "BAdd", "正在补充碳粉中"},//补粉提示
    {0x0010, "AAdd", "正在补充碳粉中"},//补粉提示
	{0x0008, "InkInjecting", "碳粉即将用尽"}, //R
	{0x0004, "PaperJamed", "卡纸"}, //R
	{0x0002, "PaperBoxAbscent", "纸盒不到位"},      //该位高表示纸盒走失 R
	{0x0001, "PaperSizeError", "纸张尺寸不符"} //ARM RW
};
#endif // !WARNING_TYPE_H
