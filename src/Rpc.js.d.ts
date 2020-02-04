
export type KeysRpcMessage = string[];

export type RpcMessage = {
    jsonrpc: string;
    method: string;
    params: KeysRpcMessage;
};

export function createKeysMessage( keys: string ): RpcMessage;