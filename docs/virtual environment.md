# virtual environments

in python on hosts where we do not have root access, we can create a virtual environment to install packages locally.

## create a virtual environment

```python
 python3 -m venv --without-pip venv  # create a virtual environment without pip
 curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py # download pip
 source venv/bin/activate     
 ```

 to exit the virtual environment, run `deactivate`.
 