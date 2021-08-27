const fs = require('fs');
const exec = require('child_process').exec;
const Web3 = require('web3');
const web3 = new Web3('http://localhost:7545');
const web3_ = new Web3('ws://localhost:7545');
const divider = '------------------------------------------------------------------';
const readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
})
async function initAccount(_web, _privateKey) {
    const myAccount = await _web.eth.accounts.privateKeyToAccount(_privateKey);
    return myAccount.address;
}
async function initContract(_web, _abi, _contractAddress) {
    const jsonInterface = JSON.parse(fs.readFileSync(_abi, 'utf8'));
    return new _web.eth.Contract(jsonInterface, _contractAddress);
}
async function listenNewTask(_contract, _address) {
    console.log(`@[${new Date().toJSON()}] Auditor begins to listen to new tasks..`);
    _contract.events.EventInformTaskInfo((error, event) => {
        const Result = event.returnValues;
        console.log(`\n${divider}\n@[${new Date().toJSON()}] EventInformAuTaskInfo received, about file: ${Result[2]}\n`);
        const auditInfo = {
            contractAddr: Result[0],
            dataOwner: Result[1],
            fileName: Result[2],
            serviceProvider: Result[3],
            blockSize: Result[4],
            blockAmount: Result[5],
            challengeAmount: Result[6],
            auditFee: Result[7]
        };
        console.log(auditInfo);
        const choiceTimer = setTimeout(() => { readline.write('N'); readline.write(null, { name: 'enter' }); }, 10000);
        readline.question(`Audit info is above.\nDo you want to accept this task(y/N): `, async function (ans) {
            if (ans == 'y') {
                clearTimeout(choiceTimer);
                const contractAddr = Result[0];
                const fileName = Result[2];
                const chalAmnt = Result[6];
                console.log(`\nApplying audit file: ${fileName}...`);
                listenAudit(contractAddr, _address, fileName, chalAmnt);
            } else {
                clearTimeout(choiceTimer);
            }
            // readline.close()
        });
    });
}
async function listenAudit(_contractAddr, _address, _fileName, _chalAmnt) {
    console.log(`@[${new Date().toJSON()}] Into listenAudit, submitting hash value 1...`);
    const _contractT1 = await initContract(web3, abiPath2, _contractAddr); // T1 for http while T2 for ws
    const _contractT2 = await initContract(web3_, abiPath2, _contractAddr);
    fs.mkdirSync(`../data/${_contractAddr}`);
    exec(`cd .. && ./challenge.out ${_contractAddr}`, (error, stdout, stderr) => {
        console.log(error || stdout || stderr, '\n');
        const buffer = fs.readFileSync(`../data/${_contractAddr}/rs.sha`);
        const param = [buffer];
        const option = { from: _address, value: '2000000000000000000' }
        _contractT1.methods.applyAudit(...param).estimateGas(option, (error, gas) => {
            console.log(`The applyAudit's estimate gas is`, gas || 0);
            _contractT1.methods.applyAudit(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
        });
    });
    _contractT2.events.EventInformSubmitHashKey1((error, event) => {
        console.log(`@[${new Date().toJSON()}] ${_fileName} EventInformSubmitHashKey1 received, submitting hash key 1...`);
        const num_r = fs.readFileSync(`../data/${_contractAddr}/r`, 'utf8');
        const num_s = fs.readFileSync(`../data/${_contractAddr}/s`, 'utf8');
        const param = [num_r, num_s];
        const option = { from: _address }
        _contractT1.methods.submitHashKey1(...param).estimateGas(option, (error, gas) => {
            console.log(`The submitHashKey1's estimate gas is`, gas || 0);
            _contractT1.methods.submitHashKey1(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
        });
    });
    _contractT2.events.EventInformSubmitHashKey2((error, event) => {
        console.log(`@[${new Date().toJSON()}] ${_fileName} EventInformSubmitHashKey2 received, submitting hash key 2...`);
        const rightP = fs.readFileSync(`../data/${_contractAddr}/right_equation`, 'utf8');
        const result = fs.readFileSync(`../data/${_contractAddr}/result`, 'utf8');
        const param = [rightP, result];
        const option = { from: _address }
        _contractT1.methods.submitHashkey2(...param).estimateGas(option, (error, gas) => {
            console.log(`The submitHashkey2's estimate gas is`, gas || 0);
            _contractT1.methods.submitHashkey2(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
        });
    });
    _contractT2.events.EventInformProofVerify((error, event) => {
        console.log(`@[${new Date().toJSON()}] ${_fileName} EventInformProofVerify received, submitting hash value 2...`);
        const Result = event.returnValues;
        fs.writeFileSync(`../data/${_contractAddr}/sum_r`, Result[0], 'utf8');
        fs.writeFileSync(`../data/${_contractAddr}/sum_s`, Result[1], 'utf8');
        fs.writeFileSync(`../data/${_contractAddr}/mu`, Buffer.from(web3.utils.hexToBytes(Result[2])));
        fs.writeFileSync(`../data/${_contractAddr}/sigma`, Buffer.from(web3.utils.hexToBytes(Result[3])));
        exec(`cd .. && ./proof_verify.out ${_fileName} ${_chalAmnt} ${_contractAddr}`, (error, stdout, stderr) => {
            console.log(error || stdout || stderr);
            const buffer = fs.readFileSync(`../data/${_contractAddr}/rightr.sha`);
            const param = [buffer];
            const option = { from: _address }
            _contractT1.methods.prepareBallot(...param).estimateGas(option, (error, gas) => {
                console.log(`The prepareBallot's estimate gas is`, gas || 0);
                _contractT1.methods.prepareBallot(...param)
                    .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
            });
        });
    });
    _contractT2.events.EventInformTaskDone((error, event) => {
        console.log(`@[${new Date().toJSON()}] EventInformTaskDone received`);
        const Result = event.returnValues;
        printConclusion(Result[0]);
    });
}
const config = require('../../config');
const privateKey = config.auPrivateKey;
const manageAddress = config.contractAddress;
const abiPath = config.abiPath;
const abiPath2 = config.abiPath2;
async function main() {
    const contractAddress = fs.readFileSync(manageAddress, 'utf8');
    const address = await initAccount(web3, privateKey);
    const contract = await initContract(web3_, abiPath, contractAddress);
    await listenNewTask(contract, address);
}
function printConclusion(result) {
    switch (result) {
        case '0': console.log('All auditors got a same audit solution TRUE, normal ends.'); break;
        case '1': console.log('All auditors got a same audit solution FALSE, normal ends.'); break;
        case '2': console.log('No valid auditors applied in the task.'); break;
        case '3': console.log('The service provider did not response to submit the proof.'); break;
        case '4': console.log('The owner arbitrate the task resulted TRUE, because auditors did not draw a same conclusion.'); break;
        case '5': console.log('The owner arbitrate the task resulted FALSE, because auditors did not draw a same conclusion.'); break;
        default: break;
    }
}
main();