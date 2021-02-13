import { connect } from 'mqtt';
import { on_connect, on_message } from './callbacks';

const client = connect( 'mqtt://test.mosquitto.org' );

client.on( 'connect', on_connect( client ) );
client.on( 'message', on_message( client ) );