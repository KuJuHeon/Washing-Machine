// I/O Registers definitions
#include <mega128a.h>

// Alphanumeric LCD functions
#include <alcd.h>
#include <stdio.h>
#include <string.h>
#include <delay.h>
#define MOTOR_ENA 5
#define MOTOR_IN1 2
#define MOTOR_IN2 3

char set[4], ch[4];
char buf_1[80], buf_2[80];
//buzzer사용  함수선언
void buzzer(float hz, int count) {
    char j;
    for (j = 0; j < count; j++) {
        PORTG = 0x01;
        delay_ms(((float)1000 / hz) / 2);

        PORTG = 0x00;
        delay_ms(((float)1000 / hz) / 2);
    }
}
void buzzer_on() {
    char i;
    for (i = 0; i < 3; i++) {
        buzzer(480, 16);
        //buzzer(320, 16);
        delay_ms(1000);
        //3번  연속으로 buzzer을 작동시키려면 "//"을 지워주세요 *^_^*
    }
}
void MOTOR_STOP() {
    PORTE &= ~(1 << MOTOR_IN1);
    PORTE &= ~(1 << MOTOR_IN2);
    PORTE &= ~(1 << MOTOR_ENA);
}
void MOTOR_MOVE(char dir) {
    //시계
    if (dir == 0) {
        PORTE |= (1 << MOTOR_IN1);
        PORTE &= ~(1 << MOTOR_IN2);
        PORTE |= (1 << MOTOR_ENA);
    }
    //반시계
    else if (dir == 1) {
        PORTE &= ~(1 << MOTOR_IN1);
        PORTE |= (1 << MOTOR_IN2);
        PORTE |= (1 << MOTOR_ENA);
    }
}
char msec = 99, sec, min, start = 0, total_time = 0, m_dir = 0;
char pause = 0, p_m, p_s, p_msec; //일시정지일 때 시간 저장(pause가 0이면 일시정지 ㄴㄴ)
char check = 1;
//타이머 오버플로우 인터럽트
interrupt[TIM1_OVF] void timer1_ovf_isr(void)
{
    //10ms 오버플로우
    TCNT1H = 0xB1;
    TCNT1L = 0xE0;
    if (start == 1) {
        //일시정지면
        if (pause == 1) {
            sec = p_s;
            min = p_m;
            msec = p_msec;
            //모터 멈추기     
        }
        //일시정지 모드가 아니면 다시 그 시간부터 진행됨(그 시간은 이제 일시정지 인터럽트에 들어있음)
        else if (pause == 0) {
            msec--;
            if (msec == 0) {
                total_time--;
                //마지막 탈수 모드는 방향 안바뀜
                if (total_time % 5 == 0 && total_time >= set[3]) {
                    MOTOR_STOP();
                    delay_ms(1000);
                    MOTOR_MOVE(m_dir);
                    m_dir = (m_dir + 1) % 2;
                }
                //현재 어떤 모드 진행 중인지 
                if (total_time > set[2] + set[3]) {
                    sprintf(buf_1, "NOW WASH...     ");
                    PORTA = 0xfd;
                }
                else if (total_time <= set[2] + set[3] && total_time > set[3]) {
                    sprintf(buf_1, "NOW RINSE...  ");
                    PORTA = 0xfb;
                }
                else {
                    sprintf(buf_1, "NOW DRY...     ");
                    PORTA = 0xf7;
                }
                //타이머 
                if (sec != 0) {
                    sec--;
                    msec = 99;
                }
                else if (sec == 0) {
                    if (min != 0) {
                        min--;
                        sec = 59;
                        msec = 99;
                    }
                    else if (min == 0) {
                        if (sec == 0 && msec == 0) {
                            sprintf(buf_1, "FINISH!!        ");
                            sprintf(buf_2, "             ");
                            MOTOR_STOP();
                            PORTA = 0xdf;
                            if (check == 1) {
                                buzzer_on();
                                check = 0;
                            }
                        }
                    }
                }
            }
        }
        sprintf(buf_2, "%02d:%02d         ", min, sec);
    }
}
//모드 변경해주는 인터럽트 d3
interrupt[EXT_INT0] void ext_int0_isr(void)
{
    delay_ms(55);
    if (set[0] == 0) {
        ch[0] = ch[0] % 2 + 1;
        //여기에 print 문자열 만들기 
        if (ch[0] == 1) { 
            sprintf(buf_1,"SELECET MODE");
            sprintf(buf_2, "AUTO");  
        }
        else if (ch[0] == 2) {
            sprintf(buf_1,"SELECET MODE");
            sprintf(buf_2, "SELF");  
        }
        delay_ms(55);
    }
    else if (set[1] == 0 && set[0] != 0) {
        ch[1] = ch[1] % 3 + 1;
        //여기에 print 문자열 만들기 
        sprintf(buf_1, "SELECT WASH ");
        sprintf(buf_2, "%d", ch[1]);
        delay_ms(55);
    }
    else if (set[2] == 0 && set[1] != 0) {
        ch[2] = ch[2] % 3 + 1;
        //여기에 print 문자열 만들기  
        sprintf(buf_1, "SELECT RINSE");
        sprintf(buf_2, "%d", ch[2]);
        delay_ms(55);
    }
    else if (set[3] == 0 && set[2] != 0) {
        ch[3] = ch[3] % 3 + 1;
        //여기에 print 문자열 만들기 
        sprintf(buf_1, "SELECT DRY  ");
        sprintf(buf_2, "%d", ch[3]);
        delay_ms(55);
    }
}

//결정한 모드 확정 짓는 인터럽트 e4
interrupt[EXT_INT1] void ext_int1_isr(void)
{
    delay_ms(55);
    if (set[0] == 0 && ch[0] != 0) {
        set[0] = ch[0];
        //모드 확정 지었으면 다음 단계로 넘어가기
        if (set[0] == 1) {
            set[1] = 20, set[2] = 20, set[3] = 20;
            ch[1] = 20, ch[2] = 20, ch[3] = 20;
            sprintf(buf_1, "COMPLETE    ");
            //sprintf(buf_2," "); 
        }
        else {
            sprintf(buf_1, "SELECT WASH ");
            //sprintf(buf_2," ",0); 
        }
        delay_ms(55);
    }
    else if (set[1] == 0 && ch[1] != 0) {
        set[1] = ch[1] * 10;
        sprintf(buf_1, "SELECT RINSE");
        //sprintf(buf_2,"%4d",0);
        delay_ms(55);
    }
    else if (set[2] == 0 && ch[2] != 0) {
        set[2] = ch[2] * 10;
        sprintf(buf_1, "SELECT DRY  ");
        //sprintf(buf_2,"%4d",0); 
        delay_ms(55);
    }
    else if (set[3] == 0 && ch[3] != 0) {
        set[3] = ch[3] * 10;
        sprintf(buf_1, "COMPLETE     ");
        //sprintf(buf_2," ",0); 
        delay_ms(55);
    }
    sprintf(buf_2, "        ");
}
char cleaning = 0;
//결정했으면 세탁기 실행시키는 인터럽트 e6
interrupt[EXT_INT2] void ext_int2_isr(void)
{
    delay_ms(55);
    //motor(set[0],set[1]*10,set[2]*10, set[3]*10);
		//설정값 있을 때만 버튼이 동작하도록 -> 오작동 방지
    if (set[0] * set[1] * set[2] * set[3] == 0) {
        sprintf(buf_1, "SELECT FIRST");
        sprintf(buf_2,"               ");
    }
    else {
        cleaning = 1;
        if (set[0] == 1) {
            //자동 모드    
            sprintf(buf_2, "AUTO %d %d %d", set[1], set[2], set[3]);
            delay_ms(55);
        }
        else if (set[0] == 2) {
            //설정한 값으로 실행 
            sprintf(buf_2, "SELF %d %d %d", set[1], set[2], set[3]);
            delay_ms(55);
        }
        start = 1; //누르면 타이머 스타트
        //시간 설정해야함
        total_time = set[1] + set[2] + set[3];
        min = (set[1] + set[2] + set[3]) / 60;
        sec = (set[1] + set[2] + set[3]) % 60;
        sprintf(buf_1, "NOW WASH...     ");
        MOTOR_MOVE(m_dir);
    }
    delay_ms(55);
}

//일시정지 인터럽트 e5
interrupt[EXT_INT4] void ext_int4_isr(void)
{
		//설정값 있을 때만 버튼이 동작하도록 -> 오작동 방지
    if (set[0] * set[1] * set[2] * set[3] == 0) {
        sprintf(buf_1, "SELECT FIRST");
        sprintf(buf_2, "               ");
    }
    else if(cleaning == 1){//일시정지 만들기
        if (pause == 0) {
            pause = 1;
            //일시정지 시간에 현재 시간 정리
            p_m = min;
            p_s = sec;
            p_msec = msec;
            sprintf(buf_1, "PAUSE           ");
            PORTA = 0xef;
            MOTOR_STOP();
        }
        //일시정지 풀기
        else {
            pause = 0;
            //sprintf(buf_1,"CLEANING        "); 
            MOTOR_MOVE(m_dir);
        }
    }
    delay_ms(55);
}
void set_init() {
    int i;
    for (i = 0; i < 4; ++i) {
        set[i] = 0;
        ch[i] = 0;
    }
}


void main(void)
{
    DDRA = 0xff;
    PORTA = 0xff;
    // Port C initialization
    // Function: Bit7=Out Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=Out 
    DDRC = (1 << DDC7) | (1 << DDC6) | (1 << DDC5) | (1 << DDC4) | (1 << DDC3) | (1 << DDC2) | (1 << DDC1) | (1 << DDC0);
    // State: Bit7=0 Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=0 
    PORTC = (0 << PORTC7) | (0 << PORTC6) | (0 << PORTC5) | (0 << PORTC4) | (0 << PORTC3) | (0 << PORTC2) | (0 << PORTC1) | (0 << PORTC0);

    // Port D initialization
    // Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
    DDRD = (0 << DDD7) | (0 << DDD6) | (0 << DDD5) | (0 << DDD4) | (0 << DDD3) | (0 << DDD2) | (0 << DDD1) | (0 << DDD0);
    // State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
    PORTD = (0 << PORTD7) | (0 << PORTD6) | (0 << PORTD5) | (0 << PORTD4) | (0 << PORTD3) | (0 << PORTD2) | (0 << PORTD1) | (0 << PORTD0);

    // Port E initialization
    // Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
    DDRE = (0 << DDE7) | (0 << DDE6) | (0 << DDE5) | (0 << DDE4) | (0 << DDE3) | (0 << DDE2) | (0 << DDE1) | (0 << DDE0);
    // State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
    PORTE = (0 << PORTE7) | (0 << PORTE6) | (0 << PORTE5) | (0 << PORTE4) | (0 << PORTE3) | (0 << PORTE2) | (0 << PORTE1) | (0 << PORTE0);

    // Port F initialization
    // Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
    DDRF = (0 << DDF7) | (0 << DDF6) | (0 << DDF5) | (0 << DDF4) | (0 << DDF3) | (0 << DDF2) | (0 << DDF1) | (0 << DDF0);
    // State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
    PORTF = (0 << PORTF7) | (0 << PORTF6) | (0 << PORTF5) | (0 << PORTF4) | (0 << PORTF3) | (0 << PORTF2) | (0 << PORTF1) | (0 << PORTF0);

    // Port G initialization
    // Function: Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
    DDRG = 0xff;
    // State: Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
    PORTG = (0 << PORTG4) | (0 << PORTG3) | (0 << PORTG2) | (0 << PORTG1) | (0 << PORTG0);


    //타이머1
    TCCR1A = 0x00;
    TCCR1B = 0x02;
    TCNT1H = 0xB1;
    TCNT1L = 0xE0;
    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK = 0x04;
    ETIMSK = 0x00;

    //인터럽트
    EICRA = (1 << ISC31) | (0 << ISC30) | (1 << ISC21) | (0 << ISC20) | (1 << ISC11) | (0 << ISC10) | (1 << ISC01) | (0 << ISC00);
    EICRB = (0 << ISC71) | (0 << ISC70) | (0 << ISC61) | (0 << ISC60) | (0 << ISC51) | (0 << ISC50) | (1 << ISC41) | (0 << ISC40);
    EIMSK = (0 << INT7) | (0 << INT6) | (0 << INT5) | (1 << INT4) | (1 << INT3) | (1 << INT2) | (1 << INT1) | (1 << INT0);
    EIFR = (0 << INTF7) | (0 << INTF6) | (0 << INTF5) | (1 << INTF4) | (1 << INTF3) | (1 << INTF2) | (1 << INTF1) | (1 << INTF0);


    // Analog Comparator initialization
    // Analog Comparator: Off
    // The Analog Comparator's positive input is
    // connected to the AIN0 pin
    // The Analog Comparator's negative input is
    // connected to the AIN1 pin
    ACSR = (1 << ACD) | (0 << ACBG) | (0 << ACO) | (0 << ACI) | (0 << ACIE) | (0 << ACIC) | (0 << ACIS1) | (0 << ACIS0);
    SFIOR = (0 << ACME);

    lcd_init(16);
    //세탁기 시작 문자열 출력
    lcd_gotoxy(0, 0);
    lcd_putsf("WELCOME");
    delay_ms(2000);
    //모드 선택 문자열 출력 
    lcd_gotoxy(0, 0);
    lcd_putsf("SELECET MODE");
    lcd_gotoxy(0, 1);
    sprintf(buf_2, "        ");
    lcd_puts(buf_2);
    // Globally enable interrupts
    #asm("sei")
        set_init();
    while (1)
    {
        lcd_gotoxy(0, 0);
        lcd_puts(buf_1);
        lcd_gotoxy(0, 1);
        lcd_puts(buf_2);
        //lcd_clear(); 
        delay_ms(100);
    }
}