using Flax.Build;

public class LinenFlaxTarget : GameProjectTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Reference the modules for game
        Modules.Add("LinenFlax");
    }
}
