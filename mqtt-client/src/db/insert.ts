import db, {
    DB_PRECISION as precision,
    DB_NAME as database,
    MEASUREMENT as measurement,
    idbData
} from './connection';

export default async ( fields: idbData ) => {
    await db.writePoints(
        [
            {
                measurement,
                fields
            }
        ],
        {
            database,
            precision
        }
    );
}