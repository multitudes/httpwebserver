# git

## get overview
To see an overview of who pushed to which branch and the latest commits on the remote repository (origin), you can use the following steps:
	
1. **Fetch All Branches**: First, fetch all branches and their latest commits from the remote repository.

   ```sh
   git fetch --all
   ```

2. **View the Commit History with Graph**: Use `git log` with the `--graph`, `--all`, and `--decorate` options to see a graphical representation of the commit history, including remote branches.

   ```sh
   git log --graph --all --decorate --oneline
   ```

3. **View Remote Branches**: To see the latest commits on each remote branch, you can use `git branch -r` to list remote branches and then `git log` for each branch.

   ```sh
   git branch -r
   ```

   For example, to see the latest commits on the `origin/main` branch:

   ```sh
   git log origin/main --oneline
   ```

### Example

1. **Fetch All Branches**:
   ```sh
   git fetch --all
   ```

2. **View the Commit History with Graph**:
   ```sh
   git log --graph --all --decorate --oneline
   ```

3. **View Remote Branches**:
   ```sh
   git branch -r
   ```

   To see the latest commits on a specific remote branch:
   ```sh
   git log origin/main --oneline
   ```

### Using GitHub or GitLab Web Interface

For a more user-friendly overview, you can use the web interface of your Git hosting service (e.g., GitHub, GitLab, Bitbucket):

1. **Go to the Repository**: Navigate to your repository on the web interface.
2. **View Branches**: Click on the "Branches" tab to see a list of branches and their latest commits.
3. **View Commit History**: Click on individual branches to see the commit history and who pushed the commits.


To include dates in the `git log` output along with the graph, all branches, and decorations, you can use the `--pretty` option to customize the log format. Here is the command:

```sh
git log --graph --all --decorate --pretty=format:'%C(yellow)%h%Creset - %C(bold blue)%an%Creset, %C(green)%ar%Creset %C(bold red)%d%Creset %n''%C(white)%s%Creset'
```

This command will show the commit graph, all branches, decorations, and include the commit hash, author name, relative date, and commit message.

### Explanation of the Format String

- `%C(yellow)%h%Creset`: Commit hash in yellow.
- `%C(bold blue)%an%Creset`: Author name in bold blue.
- `%C(green)%ar%Creset`: Relative date (e.g., "2 days ago") in green.
- `%C(bold red)%d%Creset`: Decorations (branch and tag names) in bold red.
- `%n`: New line.
- `%C(white)%s%Creset`: Commit message in white.

### Example Output

```sh
* f0d877a - (HEAD -> main, origin/main) John Doe, 2 days ago (HEAD -> main, origin/main)
|  Initial commit
* 9b1c3e1 - Jane Smith, 3 days ago (origin/feature-branch)
|  Added new feature
* 4a1b2c3 - John Doe, 4 days ago
   Fixed bug
```

### Full Command

```sh
git log --graph --all --decorate --pretty=format:'%C(yellow)%h%Creset - %C(bold blue)%an%Creset, %C(green)%ar%Creset %C(bold red)%d%Creset %n''%C(white)%s%Creset'
```

This command will give you a detailed and visually appealing log output with dates included.