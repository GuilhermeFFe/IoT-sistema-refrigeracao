import consts from './constants';
import { connect } from 'mqtt';
import { on_connect, on_message } from './mqttCallbacks';

import { turn_on, turn_off } from './mqttCallbacks/actuators';

const client = connect( consts.MQTT_BROKER_ADDR );

client.on( 'connect', on_connect( client, consts.MQTT_DATA_TOPIC ) );
client.on( 'message', on_message );

turn_on( client );

/**
 * emit_alert, must shutdown system
 */
export default (): void => {
    turn_off( client );
}