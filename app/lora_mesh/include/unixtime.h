// adapted from an Aplication Note by Microchip for a PIC 18 MCU
// http://ww1.microchip.com/downloads/en/AppNotes/01412A.pdf
// note to self: call the legal department

typedef struct
{
    int sec, min, hr;
    int year, month, date;
} DateTime;

//Code from http://www.oryx-embedded.com/doc/date__time_8c_source.html
unsigned int unixtime(DateTime date)
{
	unsigned int y, m, d, t;
    //Year
	y = date.year;
	//Month of year
	m = date.month;
	//Day of month
	d = date.date; 
	//January and February are counted as months 13 and 14 of the previous year
	if(m <= 2) {
	    m += 12;
	    y -= 1;
	}
	//Convert years to days
	t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
	//Convert months to days
	t += (30 * m) + (3 * (m + 1) / 5) + d;
	//Unix time starts on January 1st, 1970
	t -= 719561;
	//Convert days to seconds
	t *= 86400;
	//Add hours, minutes and seconds
	t += (3600 * date.hr) + (60 * date.min) + date.sec;
 
	//Return Unix time
	return t;
}

/*#define YEAR0 1970

unsigned long long unixtime(DateTime crtime, char *local,unsigned char DST)
{
    const unsigned char calendar [] = {31, 28, 31, 30,
                                       31, 30, 31, 31,
                                       30, 31, 30, 31};
    
    unsigned long long s = 0;
    //unsigned char localposition=0,foundlocal=0;                                             
    //static unsigned char k=0;
    if ((!(crtime.year%4)) && (crtime.month>2)) 
        s+=86400;

    crtime.month--;
    while (crtime.month)
    {
        crtime.month--;
        s+=(calendar[crtime.month])*86400;  
    }

    s +=((((crtime.year-YEAR0)*365)+((crtime.year-YEAR0)/4))*(unsigned long)86400)+(crtime.date-1)*(unsigned long)86400 + (crtime.hr*(unsigned long)3600)+(crtime.min*(unsigned long)60)+(unsigned long)crtime.sec; 

    return s;  // return the UNIX TIME
}
*/
