import db, { idbData } from './connection';
import consts from '../constants';
import emit_alert from '../index';

interface iMeanData {
    mean_FP: number,
    mean_I: number,
    mean_PA: number,
    mean_PB: number,
    mean_P_calc: number,
    mean_T1: number,
    mean_T2: number,
    mean_T3: number,
    mean_T4: number,
    mean_T5: number,
    mean_T6: number,
    mean_T7: number,
    mean_T8: number,
    mean_T9: number,
    mean_T_cd_calc: number,
    mean_T_ev_calc: number,
    mean_V: number,
    mean_W: number,
    mean_Wh: number,
    mean_freq: number
};

let errors: number = 0;

const diff = (n1: number, n2: number): number => {
    return (n1+n2)/( (n1+n2)/2 );
}

const on_result = ( data: idbData, mean: iMeanData ): boolean => {
    if( diff( data.W, mean.mean_P_calc ) > 0.15 ||
        diff( data.T4, mean.mean_T_ev_calc ) > 0.15 ||
        diff( data.T3, mean.mean_T_cd_calc ) > 0.15 )
    {
        if( ++errors > 5 ) // 30 wrong measurements equals about >1 minute
        {
            emit_alert( );
            return true;
        }
    }
    else
    {
        errors = 0;
    }
    return false;
}

export default async ( data: idbData ) => {
    if( data.W === 0 && data.m )
    {
        if( ++errors > 5 )
        {
            emit_alert();
            return true;
        }
    }

    const result = await db.query( `SELECT MEAN( * ) from ${consts.DB_MEASUREMENT_NAME} WHERE time > now()-5m` );
    return on_result( data, <iMeanData>result[0] );
};