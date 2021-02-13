import { MqttClient } from 'mqtt';

const on_subscribe = ( client: MqttClient, topic: string ) => ( err: Error ) => {
    if( err ) {
        throw err;
    }
    else
    {
        console.log( 'Successfully connected to MQTT broker!' );
        client.publish( topic, 'Connected!' );
    }
}

export default ( client: MqttClient, topic: string ) => ( ) => {
    client.subscribe( topic, on_subscribe( client, topic ) );
};