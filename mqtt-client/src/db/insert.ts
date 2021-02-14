import db, { idbData } from './connection';
import consts from '../constants';

export default async ( fields: idbData ) => {
    await db.writePoints(
        [
            {
                measurement: consts.DB_MEASUREMENT_NAME,
                fields
            }
        ],
        {
            database: consts.DB_NAME
        }
    );
}