import consts from './constants';
import { connect } from 'mqtt';
import { on_connect, on_message } from './mqttCallbacks';

const client = connect( consts.MQTT_BROKER_ADDR );

client.on( 'connect', on_connect( client, consts.MQTT_DATA_TOPIC ) );
client.on( 'message', on_message );
