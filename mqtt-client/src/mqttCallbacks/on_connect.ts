import { MqttClient } from 'mqtt';

const on_subscribe = ( err: Error ) => {
    if( err ) {
        throw err;
    }
    else
    {
        console.log( 'Successfully connected to MQTT broker!' );
    }
}

export default ( client: MqttClient, topic: string ) => ( ) => {
    client.subscribe( topic, on_subscribe );
};