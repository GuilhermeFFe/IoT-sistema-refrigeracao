import { MqttClient } from 'mqtt';

export default (client: MqttClient ) => ( topic: string, message: Buffer ) => {
    console.log( `topic=${topic}, message=${message.toString( )}` );
    setTimeout( () => client.publish( topic, 'Hello world!' ), 2000 );
};