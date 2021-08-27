// SPDX-License-Identifier: GPL-3.0
pragma solidity ^0.8.4;

// Because contract produced from the factory is paramatered by file name, so cocurrent auditing is supported.
contract AuditTask {
    uint256 constant MINIMAL_DEPOSIT = 2 ether;
    address public chairperson;
    uint256 public fileName;
    uint256 public challengeAmount;
    address public dataOwner;
    address public serviceProvider;
    uint256 public sum_r;
    uint256 public sum_s;
    bytes20 public mu;
    bytes public sigma;
    uint256 private finalResult;
    address[] public validAuditors1; // validAuditors1 is the auditors pass the 1st TSS.
    address[] public validAuditors2; // validAuditors2 is the auditors pass the 2nd TSS.
    enum ContractStates {
        Apply,
        SubmitHashKey1,
        SubmitProof,
        prepareBallot,
        SubmitHashKey2,
        Arbitrate,
        Done
    }
    ContractStates contractState = ContractStates.Apply;
    struct Auditor {
        uint256 deposit;
        bytes32 hashVal1;
        bytes32 hashVal2;
        uint256 num_r;
        uint256 num_s;
        uint256 num_zr;
        uint256 result;
    }
    mapping(address => Auditor) public paidAuditors;
    mapping(bytes32 => address) public hash12Auditors; // Preventing auditor A from copying the hash value submitted by B in the first TSS (Two‐stage submission).

    constructor(
        address _chairperson,
        uint256 _fileName,
        uint256 _challengeAmount,
        address _dataOwner,
        address _serviceProvider
    ) {
        fileName = _fileName;
        serviceProvider = _serviceProvider;
        chairperson = _chairperson;
        dataOwner = _dataOwner;
        challengeAmount = _challengeAmount;
    }

    fallback() external payable {}

    receive() external payable {}

    event EventInformSubmitHashKey1();
    event EventInformSubmitHashKey2();
    event EventInformProofGen(address, uint256, uint256); // provider's address, num_r, num_s
    event EventInformProofVerify(uint256, uint256, bytes20, bytes); // num_r, num_s, mu, sigma
    event EventInformArbitrate(uint256, uint256, bytes20, bytes); // num_r, num_s, mu, sigma
    event EventInformTaskDone(uint256);
    // 0. All auditors got a same audit solution TRUE, normal ends;
    // 1. All auditors got a same audit solution FALSE, normal ends;
    // 2. No valid auditors applied in the task;
    // 3. The service provider did not response to submit the proof;
    // 4. The owner arbitrate the task resulted TRUE, because auditors did not draw a same conclusion
    // 5. The owner arbitrate the task resulted FALSE, because auditors did not draw a same conclusion.

    modifier onlyGuy(address guysAddress) {
        require(
            msg.sender == guysAddress,
            "Only this guy can call this function."
        );
        _;
    }

    modifier onlyPaidAuditors() {
        require(
            paidAuditors[msg.sender].deposit >= MINIMAL_DEPOSIT,
            "Only paid auditors can call this function."
        );
        _;
    }

    modifier onlyAuPassed1stTSS() {
        require(
            paidAuditors[msg.sender].deposit >= MINIMAL_DEPOSIT &&
                paidAuditors[msg.sender].num_r > 0 &&
                paidAuditors[msg.sender].num_s > 0 &&
                paidAuditors[msg.sender].hashVal1 != "",
            "Only auditors passed 1st TSS can call this function."
        );
        _;
    }

    modifier onlyState(ContractStates _contractState) {
        require(
            contractState == _contractState,
            "Only can call this function in a proper state of the contract."
        );
        _;
    }

    modifier notState(ContractStates _contractState) {
        require(
            _contractState != contractState,
            "The method cannot be called in this state of the audit task."
        );
        _;
    }

    // Auditors apply for auditing.
    function applyAudit(bytes32 _hashVal)
        public
        payable
        onlyState(ContractStates.Apply)
    {
        require(msg.value >= MINIMAL_DEPOSIT, "INFFICIENT_DEPOSIT");
        require(
            hash12Auditors[_hashVal] == address(0x0),
            "SUBMITTED VALUE DUPLICATED"
        );
        hash12Auditors[_hashVal] = msg.sender;
        paidAuditors[msg.sender].deposit += msg.value;
        paidAuditors[msg.sender].hashVal1 = _hashVal;
    }

    function informSubmitHashKey1()
        public
        onlyGuy(chairperson)
        onlyState(ContractStates.Apply)
    {
        contractState = ContractStates.SubmitHashKey1;
        emit EventInformSubmitHashKey1();
    }

    // Paid auditors submit hash key for the first TSS.
    function submitHashKey1(uint256 _num_r, uint256 _num_s)
        public
        onlyPaidAuditors
        onlyState(ContractStates.SubmitHashKey1)
    {
        paidAuditors[msg.sender].num_r = _num_r;
        paidAuditors[msg.sender].num_s = _num_s;
        require(
            paidAuditors[msg.sender].hashVal1 ==
                sha256(abi.encodePacked(_num_r + _num_s)),
            "KEY and VALUE of Hash 1 not match."
        );
        validAuditors1.push(msg.sender);
    }

    // Inform the storage provider to generate integrity proof.
    function informProofGen()
        public
        onlyGuy(chairperson)
        onlyState(ContractStates.SubmitHashKey1)
    {
        for (uint256 i = 0; i < validAuditors1.length; i++) {
            unchecked {
                sum_r += paidAuditors[validAuditors1[i]].num_r;
                sum_s += paidAuditors[validAuditors1[i]].num_s;
            }
        }
        if (sum_r == 0 || sum_s == 0) {
            contractState = ContractStates.Done;
            payable(dataOwner).transfer(address(this).balance);
            emit EventInformTaskDone(2);
        } else {
            contractState = ContractStates.SubmitProof;
            emit EventInformProofGen(serviceProvider, sum_r, sum_s);
        }
    }

    // Storage provider submit proof.
    function submitProof(bytes20 _mu, bytes memory _sigma)
        public
        onlyGuy(serviceProvider)
        onlyState(ContractStates.SubmitProof)
    {
        require(_mu != "" && _sigma.length > 0, "Has been submitted.");
        mu = _mu;
        sigma = _sigma;
        contractState = ContractStates.prepareBallot;
        emit EventInformProofVerify(sum_r, sum_s, _mu, _sigma);
    }

    // Auditors prepare to ballot by submitting the hash value (the 2nd TSS).
    function prepareBallot(bytes32 _hashVal)
        public
        onlyAuPassed1stTSS
        onlyState(ContractStates.prepareBallot)
    {
        paidAuditors[msg.sender].hashVal2 = _hashVal;
    }

    function informSubmitHashKey2()
        public
        onlyGuy(chairperson)
        onlyState(ContractStates.prepareBallot)
    {
        if (contractState == ContractStates.prepareBallot) {
            contractState = ContractStates.SubmitHashKey2;
            emit EventInformSubmitHashKey2();
        } else {
            contractState = ContractStates.Done;
            payable(dataOwner).transfer(address(this).balance);
            emit EventInformTaskDone(3);
        }
    }

    //
    function submitHashkey2(uint256 _num_zr, uint256 _result)
        public
        onlyAuPassed1stTSS
        onlyState(ContractStates.SubmitHashKey2)
    {
        require(_result == 0 || _result == 1, "Result submitted is not 0 or 1");
        paidAuditors[msg.sender].num_zr = _num_zr;
        paidAuditors[msg.sender].result = _result;
        require(
            paidAuditors[msg.sender].hashVal2 ==
                sha256(
                    abi.encodePacked(
                        _num_zr + paidAuditors[msg.sender].num_r + _result
                    )
                ),
            "KEY and VALUE of Hash 2 not match."
        );
        validAuditors2.push(msg.sender);
    }

    function judge()
        public
        onlyGuy(chairperson)
        onlyState(ContractStates.SubmitHashKey2)
    {
        require(address(this).balance > 0, "No balance left.");
        if (isSame()) {
            for (uint256 i = 0; i < validAuditors2.length; i++) {
                payable(validAuditors2[i]).transfer(
                    paidAuditors[validAuditors2[i]].deposit
                );
            }
            uint256 eachPay = address(this).balance / validAuditors2.length;
            for (uint256 i = 0; i < validAuditors2.length; i++) {
                payable(validAuditors2[i]).transfer(eachPay);
            }
            finalResult = paidAuditors[validAuditors2[0]].result;
            contractState = ContractStates.Done;
            emit EventInformTaskDone(finalResult);
        } else {
            contractState = ContractStates.Arbitrate;
            emit EventInformArbitrate(sum_r, sum_s, mu, sigma);
        }
    }

    function isSame() internal view returns (bool) {
        if (validAuditors2.length == 0) return false;
        if (validAuditors2.length == 1) return true;
        for (uint256 i = 0; i < validAuditors2.length - 1; i++) {
            if (
                paidAuditors[validAuditors2[i]].result !=
                paidAuditors[validAuditors2[i + 1]].result
            ) return false;
        }
        return true;
    }

    function arbitrate(uint256 _num_zr, uint256 _result)
        public
        onlyGuy(dataOwner)
        onlyState(ContractStates.Arbitrate)
    {
        finalResult = _result;
        uint256 count = 0;
        for (uint256 i = 0; i < validAuditors2.length; i++) {
            if (
                paidAuditors[validAuditors2[i]].num_zr == _num_zr &&
                paidAuditors[validAuditors2[i]].result == _result
            ) {
                count++;
                payable(validAuditors2[i]).transfer(
                    paidAuditors[validAuditors2[i]].deposit
                );
            }
        }
        uint256 eachPay = address(this).balance / (count + 1);
        for (uint256 i = 0; i < validAuditors2.length; i++) {
            if (
                paidAuditors[validAuditors2[i]].num_zr == _num_zr &&
                paidAuditors[validAuditors2[i]].result == _result
            ) {
                payable(validAuditors2[i]).transfer(eachPay);
            }
        }
        payable(dataOwner).transfer(eachPay);
        contractState = ContractStates.Done;
        emit EventInformTaskDone(4);
    }

    function withdraw()
        public
        onlyGuy(chairperson)
        onlyState(ContractStates.Done)
    {
        payable(chairperson).transfer(address(this).balance);
    }
}
