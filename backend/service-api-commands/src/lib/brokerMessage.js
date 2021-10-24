export class BrokerMessage {
    /**
     * @class BrokerMessage
     *  @type {Object}
     *  @description Representa un enviado por un dispositivo o una app al broker.
     *  @property {number} command 
     *  @property {number} register 
     *  @property {number} layer 
     *  @property {number} qos 
     *  @property {number} day 
     *  @property {number} month 
     *  @property {number} year 
     *  @property {number} hour 
     *  @property {number} minutes 
     *  @property {number} seconds 
     *  @property {number} length 
     *  @property {Array|Object} payload 
     *  @property {number} checksum 
     *  @property {number} end 
     */
    constructor (command = 0, register = 0, layer = 0) {
        this.command = command;
        this.register = register;
        this.layer = layer;
        this.day = 1;
        this.month = 1;
        this.year = 21;
        this.hour = 12;
        this.minutes = 0;
        this.seconds = 0;
        this.length = 0;
        this.payload = [];
        this.checksum = 0;
    }

    /**
     * Copia el contenido de un objeto obtenido mediante el {@link Parser} de tipo {@link basicMessageParser} a un objeto de tipo {@link BrokerMessage}
     * @param {*} parsedMessage Objeto obtenido de aplicar el {@link Parser} de tipo {@link basicMessageParser} al mensaje que llegó
     */
    copyFromParsedMessage (parsedMessage) {
        this.command = parsedMessage.command;
        this.register = parsedMessage.register;
        this.layer = parsedMessage.layer;
        this.day = parsedMessage.day;
        this.month = parsedMessage.month;
        this.year = parsedMessage.year;
        this.hour = parsedMessage.hour;
        this.minutes = parsedMessage.minutes;
        this.seconds = parsedMessage.seconds;
        this.length = parsedMessage.length;
        this.payload = parsedMessage.payload;
        this.checksum = parsedMessage.checksum;
    }

    /**
     * Agrega un byte al payload del BrokerMessage
     * @param {number} data Byte a agregar al payload
     */
    addByte (data) {
        this.payload.push(data);
        this.length++;
    }

    /**
     * Agrega un array de bytes al payload del BrokerMessage
     * @param {Array} data Array de bytes a agregar al payload
     */
    addBytes (data) {
        this.payload.push(...data);
        this.length += data.length;
    }

    getBufferToSend () {
        let command = Buffer.from([this.command]);
        let register = Buffer.allocUnsafe(2);
        register.writeUInt16LE(this.register);
        let layer = Buffer.from([this.layer]);

        let currentDate = new Date();
        let date = Buffer.from([currentDate.getDate(), currentDate.getMonth() + 1, currentDate.getFullYear() - 2000]);
        let time = Buffer.from([currentDate.getHours(), currentDate.getMinutes(), currentDate.getSeconds()]);

        let length = Buffer.allocUnsafe(2);
        length.writeUInt16LE(this.length);

        let payload = Buffer.from(this.payload);

        let bufferToSend = Buffer.concat([command, register, layer, date, time, length, payload]);

        this.checksum = this.getChecksum (bufferToSend);
        let checksum = Buffer.from([this.checksum])

        bufferToSend = Buffer.concat([bufferToSend, checksum]);

        return bufferToSend;
    }


    getChecksum (buffer) {
        let sum = 0;

        buffer.forEach(data => sum += data);

        return 0xff - (sum & 0xff);
    }
}