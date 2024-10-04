{ nixpkgs
, ...
}@args:

let
  pkgs = import nixpkgs { };
  allNovaRepos = builtins.foldl' pkgs.lib.recursiveUpdate { } (builtins.attrValues (import ../../nova-repos.nix));

  inherit (import ../inputs.nix args) mkGitHubInput;

  # converts "nova" to "nova-monorepo"
  fixNovaMonorepoInputName = name: if name == "nova" then "nova-monorepo" else name;

  novaPrs = builtins.foldl'
    (allPrs: repo:
      let
        branch = allNovaRepos.${repo} or null;
        repoPrs = builtins.attrValues (builtins.fromJSON (builtins.readFile args."${repo}-pr-json"));
      in
      allPrs ++ builtins.filter
        (pr:
          # We're only interested in open PRs.
          pr.state == "open" &&

          # Ensure that the PR is targeting the expected branch, or the default branch if none is specified.
          (pr.base.ref == (if branch == null then pr.base.repo.default_branch else branch)))
        repoPrs)
    [ ]
    ([ "nova" ] ++ builtins.attrNames allNovaRepos);
in
{
  planPrJobsets = name: { description, inputs ? { }, ... }@args:
    let
      # Exclude PRs for repositories that aren't used in the jobset.
      repoWhitelist = [ "nova" ] ++ builtins.attrNames inputs;
      relevantPrs = builtins.filter (pr: builtins.elem pr.base.repo.name repoWhitelist) novaPrs;
    in
    { ${name} = args; }
    // builtins.listToAttrs (map
      (pr: pkgs.lib.nameValuePair "${name}-pr-${pr.base.repo.name}-${pr.number}" (args // {
        hidden = true;
        description = "${description} - ${pr.base.repo.name}#${toString pr.number} (${pr.title})";
        inputs = inputs // {
          # Replace the input in question with the PR's merge ref.
          ${fixNovaMonorepoInputName pr.base.repo.name} = mkGitHubInput {
            owner = pr.head.repo.owner.login;
            repo = pr.head.repo.name;
            branch = "pull/${pr.number}/merge";
          };

          # Add the link as an input, for convenience in the Web UI.
          "_pr-link" = {
            type = "string";
            value = pr.html_url;
            emailresponsible = false;
          };
        };
      }))
      relevantPrs);
}
