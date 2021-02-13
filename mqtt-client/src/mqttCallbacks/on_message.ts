import { MqttClient } from 'mqtt';
import { idbData } from '../db/connection';
import insert from '../db/insert';

const test_msg: idbData = {
    test: '0'
}

export default (client: MqttClient ) => async ( topic: string, message: Buffer ) => {
    const strMsg = message.toString( );
    console.log( `New message! topic=${topic}, message=${strMsg}` );

    try {
        const data: idbData = JSON.parse( strMsg );
        await insert( data );
    } catch ( err ) {
        console.error( err );
    }

    test_msg.test = (parseInt( test_msg.test ) + 1).toString( );
    
    setTimeout( () => client.publish( topic, JSON.stringify( test_msg ) ), 6000 );
};