import { connect } from 'mqtt';
import { on_connect, on_message } from './mqttCallbacks';

const client = connect( 'mqtt://test.mosquitto.org' );

client.on( 'connect', on_connect( client, "tmp_topic" ) );
client.on( 'message', on_message( client ) );