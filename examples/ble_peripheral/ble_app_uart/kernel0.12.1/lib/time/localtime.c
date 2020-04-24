#include <include/stdio.h>
#include <include/time.h>

int kasctime(__u32 TimeStamp, struct localtime *LocalTime) 
{
int Temp_Days,Temp_YearDays;
int Year; 
int Month;
int Day;
int Week;
short i;
short leap;

//------------------------

const short Month_Days[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
const short Month_Days_LeapYear[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};

Temp_Days = (TimeStamp/86400)+1;
if(Temp_Days<1096)
{
	Year = 1970+Temp_Days/365;
	Temp_YearDays=Temp_Days%365;
	if(Temp_Days%365==0)//判断是否为年底
	{
		Year -=1;
		Temp_YearDays=365;
	}
}
else
{
	Year = 1973 + ((Temp_Days-1096)/1461)*4 + ((Temp_Days-1096)%1461)/365;
	Temp_YearDays = ((Temp_Days-1096)%1461)%365;

	if(((Temp_Days-1096)%1461)==0)//判断是否为年底
	{
		Year -=1;
		Temp_YearDays=366;
	}
	if(((Temp_Days-1096)%1461)%365==0)//判断是否为年底
	{
		Year -=1;
		Temp_YearDays=365;
	}
}

if(((Year%4==0)&&(Year%100!=0))||(Year%400==0))   //判断是闰年
{
	for(i=12;i>0;i--)
	{
 		Temp_YearDays += Month_Days_LeapYear[i];
		if (Temp_YearDays > 366)    
		{
			Month = i;
			Day = Temp_YearDays-366;
			i=0; //结束循环
		}
	}
}

else    //判断不是闰年

{
	for(i=12;i>0;i--)
	{
 		Temp_YearDays += Month_Days[i];
		if (Temp_YearDays > 365)    
		{
			Month = i;
			Day = Temp_YearDays - 365;
			i=0;//结束循环
		}
	}
}

Week = (Day+1+2*Month+3*(Month+1)/5+Year+Year/4-Year/100+Year/400)%7;

	LocalTime->Year = Year;
	LocalTime->Month = Month;
	LocalTime->Day = Day;
	LocalTime->Week = Week;

	return 0;
}







