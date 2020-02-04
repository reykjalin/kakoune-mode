
export function handleCommand( command: string ): void;
export let currentMode: string;

export class Mode {
    tag: number;
    name: string;
}

export function getMode(): Mode;