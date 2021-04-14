import * as influx from 'influx';
import  consts from '../constants';

interface iDataRecv {
    m: boolean,
    mt: boolean,
    cd: boolean,
    vc: boolean,
    ve: boolean,
    T1: number,
    T2: number,
    T3: number,
    T4: number,
    T5: number,
    T6: number,
    T7: number,
    T8: number,
    T9: number,
    PB: number,
    PA: number,
    W: number,
    V: number,
    I: number,
    FP: number,
    Wh: number,
    freq: number
};

interface idbData extends iDataRecv {
    T_ev_calc: number,
    T_cd_calc: number,
    P_calc: number
};

const database = new influx.InfluxDB( {
    host: consts.DB_HOST,
    database: consts.DB_NAME,
    schema: [
        {
            measurement: consts.DB_MEASUREMENT_NAME,
            fields: {
                m: influx.FieldType.BOOLEAN,
                mt: influx.FieldType.BOOLEAN,
                cd: influx.FieldType.BOOLEAN,
                vc: influx.FieldType.BOOLEAN,
                ve: influx.FieldType.BOOLEAN,
                T1: influx.FieldType.FLOAT,
                T2: influx.FieldType.FLOAT,
                T3: influx.FieldType.FLOAT,
                T4: influx.FieldType.FLOAT,
                T5: influx.FieldType.FLOAT,
                T6: influx.FieldType.FLOAT,
                T7: influx.FieldType.FLOAT,
                T8: influx.FieldType.FLOAT,
                T9: influx.FieldType.FLOAT,
                PB: influx.FieldType.FLOAT,
                PA: influx.FieldType.FLOAT,
                W: influx.FieldType.FLOAT,
                V: influx.FieldType.FLOAT,
                I: influx.FieldType.FLOAT,
                FP: influx.FieldType.FLOAT,
                Wh: influx.FieldType.FLOAT,
                freq: influx.FieldType.FLOAT,
                T_ev_calc: influx.FieldType.FLOAT,
                T_cd_calc: influx.FieldType.FLOAT,
                P_calc: influx.FieldType.FLOAT
            },
            tags: []
        }
    ]
} );

database.createDatabase( consts.DB_NAME ).then( () => {
    console.log( `Database ${consts.DB_NAME} was created or already existed.` );
} ).catch( err => {
    console.log( `Error creating database ${consts.DB_NAME}` );
    throw err;
} );

export default database;
export {
    iDataRecv,
    idbData
}