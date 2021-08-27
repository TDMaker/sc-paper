const fs = require('fs');
const exec = require('child_process').exec;
const Web3 = require('web3');
const web3 = new Web3('http://localhost:7545');
const web3_ = new Web3('ws://localhost:7545');
const divider = '------------------------------------------------------------------';
async function initAccount(_web, _privateKey) {
    const myAccount = await _web.eth.accounts.privateKeyToAccount(_privateKey);
    return myAccount.address;
}
async function initContract(_web, _abi, _contractAddress) {
    const jsonInterface = JSON.parse(fs.readFileSync(_abi, 'utf8'));
    return new _web.eth.Contract(jsonInterface, _contractAddress);
}
async function listenNewTask(_contract, _address) {
    console.log(`@[${new Date().toJSON()}] Storage provider begins to listen to new tasks...\n`);
    _contract.events.EventInformTaskInfo(async function (error, event) {
        const Result = event.returnValues;
        console.log(`\n${divider}\n@[${new Date().toJSON()}] EventInformTaskInfo received, about file: ${Result[2]}\n`);
        if (Result[3] == _address) {
            const contractAddr = Result[0];
            const fileName = Result[2];
            const chalAmnt = Result[6];
            console.log(`@[${new Date().toJSON()}] it's my job, begin to listenProofGen...\n`);
            const contractT1 = await initContract(web3, abiPath2, contractAddr); // T1 for http while T2 for ws
            const contractT2 = await initContract(web3_, abiPath2, contractAddr);
            contractT2.events.EventInformTaskDone((error, event) => {
                console.log(`@[${new Date().toJSON()}] EventInformTaskDone received`);
                // console.log(event);
            });
            contractT2.events.EventInformProofGen((error, event) => {
                console.log(`@[${new Date().toJSON()}] EventInformProofGen received...`);
                const Result = event.returnValues;
                fs.mkdirSync(`../data/${contractAddr}`);
                fs.writeFileSync(`../data/${contractAddr}/sum_r`, Result[0], 'utf8');
                fs.writeFileSync(`../data/${contractAddr}/sum_s`, Result[1], 'utf8');
                exec(`cd .. && ./proof_gen.out ${fileName} ${chalAmnt} ${contractAddr}`, (error, stdout, stderr) => {
                    console.log(error || stdout || stderr, '\n');
                    const buffer_mu = fs.readFileSync(`../data/${contractAddr}/mu`);
                    const buffer_sigma = fs.readFileSync(`../data/${contractAddr}/sigma`);
                    const param = [buffer_mu, web3.utils.bytesToHex(buffer_sigma)];
                    const option = { from: _address }
                    contractT1.methods.submitProof(...param).estimateGas(option, (error, gas) => {
                        console.log(`The submitProof's estimate gas is `, gas || 0);
                        contractT1.methods.submitProof(...param)
                            .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
                    });
                });
            });
        } else {
            console.log(`That's not my job.`);
        }
    });
}
const config = require('../../config');
const privateKey = config.spPrivateKey;
const manageAddress = config.contractAddress;
const abiPath = config.abiPath;
const abiPath2 = config.abiPath2;
async function main() {
    const contractAddress = fs.readFileSync(manageAddress, 'utf8');
    const address = await initAccount(web3, privateKey);
    const contract = await initContract(web3_, abiPath, contractAddress);
    await listenNewTask(contract, address);
}
main();