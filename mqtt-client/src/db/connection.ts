import * as influx from 'influx';
import  consts from '../constants';

interface idbData {
    motor: boolean,
    vent_condensador: boolean,
    vent_evaporador: boolean,
    succao: number,
    descarga: number,
    filtro_secador: number,
    entrada_evaporador: number,
    saida_evaporador: number,
    linha_liquido: number,
    meio_evaporador: number,
    ambiente: number,
    compressor: number,
    pressao_baixa: number,
    pressao_alta: number,
    potencia: number,
    tensao: number,
    corrente: number,
    fator_potencia: number,
    watt_hora: number,
    frequencia: number
};

const database = new influx.InfluxDB( {
    host: consts.DB_HOST,
    database: consts.DB_NAME,
    schema: [
        {
            measurement: consts.DB_MEASUREMENT_NAME,
            fields: {
                motor: influx.FieldType.BOOLEAN,
                vent_condensador: influx.FieldType.BOOLEAN,
                vent_evaporador: influx.FieldType.BOOLEAN,
                succao: influx.FieldType.FLOAT,
                descarga: influx.FieldType.FLOAT,
                filtro_secador: influx.FieldType.FLOAT,
                entrada_evaporador: influx.FieldType.FLOAT,
                saida_evaporador: influx.FieldType.FLOAT,
                linha_liquido: influx.FieldType.FLOAT,
                meio_evaporador: influx.FieldType.FLOAT,
                ambiente: influx.FieldType.FLOAT,
                compressor: influx.FieldType.FLOAT,
                pressao_baixa: influx.FieldType.FLOAT,
                pressao_alta: influx.FieldType.FLOAT,
                potencia: influx.FieldType.FLOAT,
                tensao: influx.FieldType.FLOAT,
                corrente: influx.FieldType.FLOAT,
                fator_potencia: influx.FieldType.FLOAT,
                watt_hora: influx.FieldType.FLOAT,
                frequencia: influx.FieldType.FLOAT
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
    idbData
}