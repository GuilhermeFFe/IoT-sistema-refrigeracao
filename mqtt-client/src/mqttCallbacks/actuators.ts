import { MqttClient } from 'mqtt';
import constants from '../constants';

interface iCommands {
    cd?: boolean,
    cvc?: boolean,
    cc?: boolean,
    cve?: boolean, 
    mt?: boolean
};

const execute_command = (client: MqttClient, cmd: iCommands): void => {
    client.publish( constants.MQTT_CMD_TOPIC, JSON.stringify( cmd ) );
}

const turn_off = (client: MqttClient): void => {
    console.error( 'ALERT! Shutting down system!' );
    execute_command( client, { cc: false } );
};

const turn_on = (client: MqttClient): void => {
    execute_command( client, { cc: true, cd:true } );
}

export {
    turn_off,
    turn_on
}