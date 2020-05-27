// @ts-ignore: no typing
import UntypedMpyCross from './mpy-cross';

interface MpyCrossModule extends EmscriptenModule {
    (moduleOverrides?: {
        arguments: string[];
        inputFileContents: string;
        callback: (
            status: number,
            out: string,
            err: string,
            mpy?: Uint8Array
        ) => void;
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
    out: string;
    /**
     * The captured stderr.
     */
    err: string;
}

/**
 * Compiles a MicroPython source code file using mpy-cross.
 * @param fileContents The contents of the .py file to be compile.
 * @param fileName The name of the .py file (including file extension).
 * @param options Command line arguments for mpy-cross.
 */
export function compile(
    fileName: string,
    fileContents: string,
    options?: string[]
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
                callback: (status, out, err, mpy) =>
                    resolve({ status, mpy, out, err }),
            });
        } catch (err) {
            reject(err);
        }
    });
}
