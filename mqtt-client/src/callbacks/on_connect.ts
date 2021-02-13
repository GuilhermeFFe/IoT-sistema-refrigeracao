import { MqttClient } from 'mqtt';

const on_subscribe = ( client: MqttClient ) => ( err: Error ) => {
    if( err ) {
        throw err;
    }
    else
    {
        console.log( 'Connected!' );
        client.publish( 'presence', 'Hello from mqtt!' );
    }
}

export default ( client: MqttClient ) => ( ) => {
    client.subscribe( 'presence', on_subscribe( client ) );
};