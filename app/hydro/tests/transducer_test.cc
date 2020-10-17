#include <transducer.h>
#include <alarm.h>

using namespace EPOS;

OStream cout;

static unsigned long sample_time = 5*1000000;

int main()
{
    Alarm::delay(2000000);
    OStream cout;
    cout << "main()" << endl;

	// Tranducers
    // Pressure_Sensor_Keller_Capacitive level_Keller_sensor(0, Hydro_Board::Port::P6, Hydro_Board::Port::P6);
    // Water_Level_Sensor_Ultrasonic_Microflex level_Microflex_sensor(0, Hydro_Board::Port::P3, Hydro_Board::Port::P3);
    // Water_Turbidity_Sensor_OBS turbidity_OBS_sensor_high(0,Hydro_Board::Port::P6,Hydro_Board::Port::P6);
    // Water_Turbidity_Sensor_OBS turbidity_OBS_sensor_low(1,Hydro_Board::Port::P6,Hydro_Board::Port::P5);
    // new Lux_Sensor(new ADC(ADC::SINGLE_ENDED_ADC4));
    // new Switch_Sensor(0, 'B', 4, GPIO::IN, GPIO::UP, GPIO::BOTH);

    // Smart_datas
    // Pressure_Keller_Capacitive level_Keller(0, sample_time, Pressure_Keller_Capacitive::PRIVATE);
    // Water_Level_Ultrasonic_Microflex level_Microflex(0, sample_time, Water_Level_Ultrasonic_Microflex::PRIVATE);
    // Water_Turbidity_OBS turbidity_OBS_high(0, sample_time, Water_Turbidity_OBS::PRIVATE);
    // Water_Turbidity_OBS turbidity_OBS_low(1, sample_time, Water_Turbidity_OBS::PRIVATE);
    // Rain pluviometer(0, sample_time, static_cast<Rain::Mode>(Rain::PRIVATE | Rain::CUMULATIVE));
    // Luminous_Intensity lux(0, sample_time, Luminous_Intensity::PRIVATE);
    // Presence presence(0, sample_time, Presence::PRIVATE);
    // I2C_Temperature temp(0, 2000000, Luminous_Intensity::PRIVATE);
    Water_Flow_M170 flow(0, 2*sample_time, Water_Flow_M170::PRIVATE);

    while(true)
    {
        Alarm::delay(1000000);
        // cout << "level_Keller: " << level_Keller << endl;
        // cout << "level_Microflex: " << level_Microflex << endl;
        // cout << "Turbidity_OBS_high: " << turbidity_OBS_high << endl;
        // cout << "Turbidity_OBS_low: " << turbidity_OBS_low << endl;
        // cout << "Pluviometer: " << pluviometer << endl;
        // cout << "lux: " << lux << endl;
        // cout << "presenca: " << static_cast<Presence::Value>(presence) << endl;
        // cout << temp << endl;
        cout << flow << endl;
    }
    
    return 0;
}

