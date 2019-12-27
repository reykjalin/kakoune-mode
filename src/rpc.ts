
export class RPCMessage {
    jsonrpc: string = '2.0';
    method: string;
    params: [any];

    constructor(method: string, params: [any]) {
        this.method = method;
        this.params = params;
    }
}

export class KeysMessage extends RPCMessage {
    constructor(keys: string) {
        keys = keys.replace('\n', '<ret>');
        super('keys', [keys]);
    }
}