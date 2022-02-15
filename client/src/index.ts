import { SerialPort } from "serialport";
import Crypto from "crypto";

const port = new SerialPort(
  {
    path: "COM5",
    baudRate: 9600,
  },
  (err) => {
    if (err) console.log(err);
    else console.log("open");
  }
);

port.on("error", (err) => {
  console.log("Error: ", err.message);
});

//so this shit is not called again if not all data is read bruuu
// port.on("readable", () => {
//   //   let output = port.read() as Buffer;
//   //   console.log(output);

//   //THIS SHIT FAILED BECAUSE OF x00 byte apparently bruuuuuuuuhhhhhhh
//   //wait no why
//   //it's only strings anyway????
//   //nah anyway it's prolly some weird ass char that fucks up the string bruh
//   //ok this is utter bullshit
//   //console.log(new TextDecoder().decode(output));

//   //   console.log(output.toString("hex"));
//   //   return;

//   let operation = (port.read(1) as Buffer)[0];

//   console.log("op", operation.toString(16));

//   switch (operation) {
//     case 0x01:
//       {
//         console.log("ready");
//         port.write(Buffer.from([0x20, 0x21]));
//       }
//       break;
//     case 0x10:
//       {
//         let output: Buffer = port.read();

//         let enc = output.slice(0, 32);
//         let input = output.slice(32, 32 + 32);
//         let pass = output.slice(32 + 32, 32 + 32 + 256);

//         console.log(output.length, 32 + 32 + 256);

//         console.log(enc.toString("hex"));
//         console.log(input.toString("hex"));
//         console.log(pass.toString("hex"));
//       }
//       break;
//     default:
//       {
//         let output = port.read();
//         //console.log(output);
//       }
//       break;
//   }
// });

port.on("data", (data: Buffer) => {
  //console.log("Data:", data.join(" "));
  console.log(new TextDecoder().decode(data));
  //   console.log(
  //     data
  //       .toString("hex")
  //       .match(/[a-f0-9]{2}\B/g)
  //       .join(" ")
  //   );
});

//port.write(Buffer.from("gTestPasswordBruh"));

//apparently buffer ignores \0 in strings bruuuu
port.write(Buffer.from("sTestPasswordBruh"));
port.write(Buffer.from([0]));
port.write(Buffer.from(new Uint8Array(32).fill(0x69, 0, 32)));
// port.drain();
