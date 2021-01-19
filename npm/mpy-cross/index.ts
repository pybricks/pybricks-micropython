// @ts-ignore: no typing
import UntypedMpyCross from './mpy-cross';

interface MpyCrossModule extends EmscriptenModule {
    (moduleOverrides?: {
        arguments: string[];
        inputFileContents: string;
        callback: (
            status: number,
            mpy: Uint8Array | undefined,
            out: string[],
            err: string[],
        ) => void;
        locateFile(path: string, scriptDirectory: string): string;
    }): this;
    fileContents: string;
}

const MpyCross = UntypedMpyCross as MpyCrossModule;

export interface CompileResult {
    /**
     * The mpy-cross program exit code.
     */
    status: number;
    /**
     * The compiled .mpy file on success, otherwise undefined.
     */
    mpy?: Uint8Array;
    /**
     * The captured stdout.
     */
    out: string[];
    /**
     * The captured stderr.
     */
    err: string[];
}

/**
 * Compiles a MicroPython source code file using mpy-cross.
 * @param fileContents The contents of the .py file to be compile.
 * @param fileName The name of the .py file (including file extension).
 * @param options Command line arguments for mpy-cross.
 * @param wasmPath Path to location of `mpy-cross.wasm`.
 */
export function compile(
    fileName: string,
    fileContents: string,
    options?: string[],
    wasmPath?: string
): Promise<CompileResult> {
    return new Promise<CompileResult>((resolve, reject) => {
        try {
            const args = [fileName];
            if (options) {
                args.splice(0, 0, ...options);
            }
            MpyCross({
                arguments: args,
                inputFileContents: fileContents,
                callback: (status, mpy, out, err) =>
                    resolve({ status, mpy, out, err }),
                locateFile: (path, scriptDirectory) => {
                    if (path === 'mpy-cross.wasm' && wasmPath !== undefined) {
                        return wasmPath;
                    }
                    return scriptDirectory + path;
                },
            });
        } catch (err) {
            reject(err);
        }
    });
}
