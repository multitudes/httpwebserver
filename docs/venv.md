# how to handle venv on school computers

so i do not have sudo rights

i can install venv without pip
```
python3 -m venv --without-pip venv
```

So this creates a venv directory without pip
I download get-pip.py and run it
```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py
```

evenbtually deactivate the venv and activate it again
```
source venv/bin/activate
```

Now:
```
venv/bin/pip  install pytest 
which pytest  # Should show `venv/bin/pytest`
```

Run make but better to start it outside the env first
Sometimes i get twice the (venv) prefix in the terminal but deactivate doesnt work

Eventually need to install again the required packages
```
pip install -r requirements.txt
```
# or
```
venv/bin/pip install requests
```

## in the makefile

we do not use `source venv/bin activate` because make runs each command in a separate shell, so source wouldn’t persist.