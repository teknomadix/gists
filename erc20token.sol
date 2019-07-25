pragma solidity ^0.5.1;


contract ERC20Token {
    
    uint256 constant private MAX_UINT256 = 2**256 -1;

    mapping ( address => mapping ( address => uint256 )) public allowances;
    mapping ( address => uint256) public balances;

    event Transfer(address indexed _from, address indexed _to, uint256 _value);
    event Approval(address indexed _owner, address indexed _spender, uint256 _value);

    
    uint256 public totalSupply;
    string public name; // .name()
    string public symbol;
    uint8 public decimals;
    
    constructor(uint256 _totalSupply, string memory _name, string memory _symbol, uint8 _decimals)  public {
        balances[msg.sender] = _totalSupply;
        totalSupply = _totalSupply;
        name = _name;
        symbol = _symbol;
        decimals = _decimals;
    }


    function balanceOf(address _owner) public view returns(uint256 balance) {
        return balances[_owner];
    }


    function transfer(address _to, uint256 _value) public returns(bool success) {
        // prevents overspending

        require (balances[msg.sender] >= _value);

        // update balances
        balances[msg.sender] -= _value;
        balances[_to] += _value;

        return true;
        
    }

    function transferFrom(address _from, address _to, uint256 _value) public returns (bool success) {
     
            // get allowance
            uint256 allowance = allowances[_from][msg.sender];
            
            // require that value be < balances and allowance
            require (balances[_from] >= _value && allowance >= _value);
        
            // update balances
            balances[msg.sender] -= _value;
            balances[_to] += _value;

            emit Transfer(msg.sender, _to, _value);
        
            // check that allowance < MAX_UINT256
            if(allowance < MAX_UINT256)  {
                
                // deduct from allowance
                allowances[_from][msg.sender] -= _value;
            }
            emit Transfer(_from, _to, _value);    
            return true;
        }
    
    function approve(address _spender, uint256 _value) public returns (bool success){
        allowances[msg.sender][_spender] = _value;
        emit Approval(msg.sender, _spender, _value);
        return true;

    }

    function allowance(address _owner, address _spender) public view returns (uint256 remaining){
        return allowances[_owner][_spender];
    }


}
