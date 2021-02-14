import { idbData } from '../db/connection';
import insert from '../db/insert';

export default async ( topic: string, message: Buffer ) => {
    const strMsg = message.toString( ).replace( / /g, '_' );
    console.log( `New message from topic ${topic}, message=${strMsg}` );

    try {
        const data: idbData = JSON.parse( strMsg );
        await insert( data );
    } catch ( err ) {
        console.error( err );
    }
};