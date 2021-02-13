import * as influx from 'influx';

const DB_HOST = 'localhost';
const DB_PRECISION = 's';
const DB_NAME = 'refrig_measurements';
const MEASUREMENT = 'sensor_values';

interface idbData {
    test: string;
};

const database = new influx.InfluxDB( {
    host: DB_HOST,
    database: DB_NAME,
    schema: [
        {
            measurement: MEASUREMENT,
            fields: {
                test: influx.FieldType.STRING
            },
            tags: []
        }
    ]
} );

database.createDatabase( DB_NAME ).then( () => {
    console.log( `Database ${DB_NAME} was created or already existed.` );
} ).catch( err => {
    console.log( `Error creating database ${DB_NAME}` );
    throw err;
} );

export default database;
export {
    DB_HOST,
    DB_NAME,
    DB_PRECISION,
    MEASUREMENT,
    idbData
}