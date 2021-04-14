import { iDataRecv, idbData } from '../db/connection';
import insert from '../db/insert';
import emit_alert from '../db/emit_alert';

const calculate_t_ev = ( data: iDataRecv ): number => {
    return 5
         - 59.9235
         + 47.7325*(data.PB+1)
         - 17.8049*((data.PB+1)**2)
         + 3.86138*((data.PB+1)**3)
         - 0.329592*((data.PB+1)**4);
}

const calculate_t_cd = ( data: iDataRecv ): number => {
    return - 26.6328
           + 11.3174*(data.PA+1.015)
           - 0.691216*((data.PA + 1.015)**2)
           + 0.0259686*((data.PA + 1.015)**3)
           - 0.000396834*((data.PA + 1.015)**4);
}

const calculate_p = ( data: iDataRecv ): number => {
    const T_cd = calculate_t_cd( data );
    const T_ev = calculate_t_ev( data );
    return 82.4941434
         - 0.537328089*T_ev
         - 0.0626019814*(T_ev**2)
         - 0.000433566434*(T_ev**3)
         + 0.941134033*T_cd
         + 0.00347902098*(T_ev**2)
         - 0.0000151515151*(T_cd**3)
         + 0.0384205128*T_ev*T_cd
         + 0.000407342657*T_ev*(T_cd**2)
         + 0.00105920745*(T_ev**2)*T_cd
         + 0.00000104895104*(T_ev**2)*(T_cd**2);
}

const calculate_values = ( data: iDataRecv ): idbData => {
    return {
        ...data,
        T_ev_calc: calculate_t_ev( data ),
        T_cd_calc: calculate_t_cd( data ),
        P_calc: calculate_p( data )
    }
}

let measurement_padding = 150; // Used to ignore weird measurements when the engine turns on

export default async ( topic: string, message: Buffer ) => {
    const strMsg = message.toString( ).replace( / /g, '_' );
    
    try {
        const to_insert: iDataRecv = JSON.parse( strMsg );
        const data: idbData = calculate_values( to_insert );
        if( measurement_padding === 0) {
            emit_alert( data );
        }
        else {
            measurement_padding--;
        }
        
        await insert( data );
    } catch ( err ) {
        console.error( err );
    }
};